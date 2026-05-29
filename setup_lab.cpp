/*
 * ============================================================
 *  SETUP_LAB.CPP  —  Preparação do Ambiente de Laboratório
 * ============================================================
 *
 *  PROPÓSITO EDUCACIONAL:
 *  Cria a pasta de teste e arquivos fictícios que serão usados
 *  para demonstrar como um ransomware enumera e cifra vítimas.
 *
 *  COMPILE:  g++ setup_lab.cpp -o setup_lab.exe
 *  EXECUTE:  ./setup_lab.exe
 * ============================================================
 */

#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────
//  Pasta raiz do laboratório (NUNCA altere para
//  caminhos de sistema como C:\Windows ou C:\Users)
// ─────────────────────────────────────────────────
const std::string LAB_ROOT = "C:\\RansomwareTest";

// Arquivos fictícios que simulam documentos de uma vítima real
struct FakeFile {
    std::string path;
    std::string content;
};

std::vector<FakeFile> fakeFiles = {
    { LAB_ROOT + "\\documentos\\relatorio_financeiro.txt",
      "RELATORIO Q1 2025\nReceita: R$ 500.000\nDespesas: R$ 320.000\nLucro: R$ 180.000" },

    { LAB_ROOT + "\\documentos\\senhas_backup.txt",
      "Sistema ERP: admin/P@ss123\nVPN Corporativa: vpn_user/Secret456\nEmail: user@corp.com/Mail789" },

    { LAB_ROOT + "\\fotos\\ferias_familia.txt",
      "[simulacao de arquivo de imagem JPEG - dados binarios aqui]" },

    { LAB_ROOT + "\\projetos\\codigo_fonte.cpp",
      "#include <iostream>\nint main() { std::cout << 'Hello World'; return 0; }" },

    { LAB_ROOT + "\\projetos\\banco_dados.sql",
      "SELECT * FROM clientes;\nINSERT INTO pedidos VALUES (1, 'Produto A', 99.90);" },

    { LAB_ROOT + "\\backup\\config_sistema.ini",
      "[Database]\nHost=192.168.1.100\nPort=5432\nPassword=db_secret_2025" },
};

// ─────────────────────────────────────────────────
//  Cria diretório recursivamente (equivalente mkdir -p)
// ─────────────────────────────────────────────────
void criarDiretorio(const std::string& caminho) {
    // CreateDirectoryA falha silenciosamente se já existir — OK para nosso lab
    CreateDirectoryA(caminho.c_str(), NULL);
}

// ─────────────────────────────────────────────────
//  Extrai o diretório pai de um caminho completo
//  Ex: "C:\foo\bar\file.txt" → "C:\foo\bar"
// ─────────────────────────────────────────────────
std::string diretorioPai(const std::string& caminho) {
    size_t pos = caminho.find_last_of("\\/");
    if (pos == std::string::npos) return ".";
    return caminho.substr(0, pos);
}

int main() {
    std::cout << "=== Setup do Laboratório de Ransomware ===" << std::endl;
    std::cout << "Pasta raiz: " << LAB_ROOT << std::endl << std::endl;

    // 1. Cria a estrutura de pastas
    criarDiretorio(LAB_ROOT);
    criarDiretorio(LAB_ROOT + "\\documentos");
    criarDiretorio(LAB_ROOT + "\\fotos");
    criarDiretorio(LAB_ROOT + "\\projetos");
    criarDiretorio(LAB_ROOT + "\\backup");

    // 2. Escreve os arquivos fictícios
    for (const auto& f : fakeFiles) {
        std::ofstream out(f.path);
        if (out.is_open()) {
            out << f.content;
            out.close();
            std::cout << "[+] Criado: " << f.path << std::endl;
        } else {
            std::cerr << "[-] Falha ao criar: " << f.path << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << "Lab pronto! " << fakeFiles.size() << " arquivos criados." << std::endl;
    std::cout << "Agora execute ransomware_sim.exe para a demonstracao." << std::endl;

    std::cout << std::endl;
    system("pause");
    return 0;
}
