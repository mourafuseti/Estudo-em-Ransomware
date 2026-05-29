/*
 * ============================================================
 *  DECRYPTOR.CPP  —  Ferramenta de Recuperação / Análise
 * ============================================================
 *
 *  PROPÓSITO DUPLO:
 *  1. Recupera os arquivos cifrados pelo ransomware_sim
 *  2. Demonstra como um analista ou empresa de IR (Incident
 *     Response) constrói uma ferramenta de decriptação
 *
 *  CONTEXTO REAL:
 *  Empresas como Kaspersky, Emsisoft e No More Ransom publicam
 *  decryptors gratuitos quando:
 *   a) Obtêm a chave via infiltração em servidor C2
 *   b) Encontram falha criptográfica (ex: IV fixo, seed previsível)
 *   c) Fazem engenharia reversa completa da amostra
 *
 *  COMPILE:  g++ decryptor.cpp -o decryptor.exe
 *  EXECUTE:  ./decryptor.exe
 * ============================================================
 */

#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>

// ─────────────────────────────────────────────────
//  Deve ser idêntico ao ransomware_sim.cpp
//  Em um caso real, o analista extrai estas constantes
//  via engenharia reversa (IDA, Ghidra, x64dbg)
// ─────────────────────────────────────────────────
const std::string PASTA_LAB     = "C:\\RansomwareTest";
const std::string EXTENSAO      = ".ENCLAB";
const std::string MAGIC_HEADER  = "ENCLAB_V1";
const std::vector<BYTE> CHAVE   = { 0x4C, 0x41, 0x42, 0x4F, 0x52, 0x41, 0x54, 0x4F };

// ═══════════════════════════════════════════════════════════
//  COMO UM ANALISTA EXTRAI ESTAS CONSTANTES:
//
//  1. CHAVE XOR via Ghidra/IDA:
//     - Procura por arrays de bytes próximos a XOR instructions
//     - Ou usa x64dbg: breakpoint em WriteFile, inspeciona o
//       buffer antes de ser escrito — está cifrado
//     - Breakpoint em ReadFile do arquivo original — está em claro
//     - Diferença entre os dois buffers = keystream = chave
//
//  2. MAGIC BYTES via strings:
//     - $ strings ransomware_sim.exe | grep -i "enc\|lab\|cry"
//
//  3. EXTENSÃO via strings ou análise dinâmica:
//     - Process Monitor: filtro "Path ends with .ENCLAB"
// ═══════════════════════════════════════════════════════════

std::vector<BYTE> lerArquivo(const std::string& caminho) {
    std::ifstream f(caminho, std::ios::binary);
    if (!f.is_open()) return {};
    return std::vector<BYTE>(
        std::istreambuf_iterator<char>(f),
        std::istreambuf_iterator<char>()
    );
}

bool escreverArquivo(const std::string& caminho, const std::vector<BYTE>& dados) {
    std::ofstream f(caminho, std::ios::binary);
    if (!f.is_open()) return false;
    f.write(reinterpret_cast<const char*>(dados.data()), dados.size());
    return f.good();
}


// ═══════════════════════════════════════════════════════════
//  DECIFRA UM ARQUIVO .ENCLAB
//
//  ESTRUTURA DO ARQUIVO CIFRADO:
//  ┌─────────────────────────┐
//  │ MAGIC_HEADER  (9 bytes) │ ← identificador da família
//  ├─────────────────────────┤
//  │ Tamanho original (8 b)  │ ← uint64_t little-endian
//  ├─────────────────────────┤
//  │ Dados cifrados (N bytes)│ ← XOR com CHAVE
//  └─────────────────────────┘
// ═══════════════════════════════════════════════════════════
bool decifrarArquivo(const std::string& caminhoCifrado) {
    // 1. Lê o arquivo cifrado
    std::vector<BYTE> conteudo = lerArquivo(caminhoCifrado);
    size_t headerSize = MAGIC_HEADER.size(); // 9 bytes
    size_t metaSize   = headerSize + 8;      // +8 bytes do tamanho original

    if (conteudo.size() < metaSize) {
        std::cerr << "  [ERRO] Arquivo muito pequeno: " << caminhoCifrado << std::endl;
        return false;
    }

    // 2. Valida o magic header (garante que este decryptor é para esta família)
    // ANÁLISE: Um decryptor robusto SEMPRE valida o header antes de operar
    for (size_t i = 0; i < headerSize; i++) {
        if (conteudo[i] != static_cast<BYTE>(MAGIC_HEADER[i])) {
            std::cerr << "  [ERRO] Magic header invalido: " << caminhoCifrado << std::endl;
            return false;
        }
    }

    // 3. Lê o tamanho original (8 bytes little-endian após o header)
    uint64_t tamanhoOriginal = 0;
    for (int i = 0; i < 8; i++) {
        tamanhoOriginal |= static_cast<uint64_t>(conteudo[headerSize + i]) << (i * 8);
    }

    // 4. Extrai apenas os dados cifrados (após o cabeçalho completo)
    std::vector<BYTE> dadosCifrados(conteudo.begin() + metaSize, conteudo.end());

    // 5. Decifra com XOR (operação idêntica à cifra — propriedade do XOR)
    std::vector<BYTE> dadosOriginais(dadosCifrados.size());
    for (size_t i = 0; i < dadosCifrados.size(); i++) {
        dadosOriginais[i] = dadosCifrados[i] ^ CHAVE[i % CHAVE.size()];
    }

    // 6. Trunca ao tamanho original (remove possível padding)
    if (tamanhoOriginal <= dadosOriginais.size()) {
        dadosOriginais.resize(tamanhoOriginal);
    }

    // 7. Determina o nome do arquivo original (remove extensão .ENCLAB)
    std::string caminhoOriginal = caminhoCifrado.substr(
        0, caminhoCifrado.size() - EXTENSAO.size()
    );

    // 8. Salva o arquivo recuperado
    if (!escreverArquivo(caminhoOriginal, dadosOriginais)) {
        std::cerr << "  [ERRO] Falha ao salvar: " << caminhoOriginal << std::endl;
        return false;
    }

    // 9. Remove o arquivo cifrado (cleanup pós-recuperação)
    DeleteFileA(caminhoCifrado.c_str());

    return true;
}


// ═══════════════════════════════════════════════════════════
//  VARREDURA RECURSIVA — igual ao ransomware, mas busca .ENCLAB
// ═══════════════════════════════════════════════════════════
int varrerEDecifrar(const std::string& diretorio) {
    int contador = 0;
    WIN32_FIND_DATAA findData;
    std::string padrao = diretorio + "\\*";

    HANDLE hFind = FindFirstFileA(padrao.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return 0;

    do {
        std::string nome = findData.cFileName;
        if (nome == "." || nome == "..") continue;

        std::string caminho = diretorio + "\\" + nome;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            contador += varrerEDecifrar(caminho);
        } else {
            // Processa apenas arquivos .ENCLAB
            if (nome.size() > EXTENSAO.size() &&
                nome.substr(nome.size() - EXTENSAO.size()) == EXTENSAO) {

                if (decifrarArquivo(caminho)) {
                    std::cout << "  [RECUPERADO] " << caminho << std::endl;
                    contador++;
                }
            }
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
    return contador;
}


int main() {
    std::cout << "============================================" << std::endl;
    std::cout << " DECRYPTOR — Ferramenta de Recuperacao" << std::endl;
    std::cout << " Curso: Defesa Cibernetica - Malware Analysis" << std::endl;
    std::cout << "============================================" << std::endl << std::endl;

    std::cout << "[*] Buscando arquivos .ENCLAB em: " << PASTA_LAB << std::endl << std::endl;

    int total = varrerEDecifrar(PASTA_LAB);

    std::cout << std::endl;
    std::cout << "[*] Arquivos recuperados: " << total << std::endl;

    if (total == 0) {
        std::cout << "[!] Nenhum arquivo .ENCLAB encontrado." << std::endl;
        std::cout << "    Execute setup_lab.exe + ransomware_sim.exe primeiro." << std::endl;
    } else {
        std::cout << "[+] Recuperacao concluida com sucesso!" << std::endl;
    }

    std::cout << "============================================" << std::endl;

    std::cout << std::endl;
    system("pause");
    return 0;
}
