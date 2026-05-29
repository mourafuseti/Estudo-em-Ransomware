/*
 * ============================================================
 *  LOCKSCREEN.CPP  —  Tela de Resgate (Lock Screen Educacional)
 * ============================================================
 *
 *  REPLICA VISUAL de ransomwares famosos:
 *    - WannaCry  (2017) — tela vermelha, countdown, Bitcoin
 *    - Petya     (2016) — skull ASCII, tela preta
 *    - LockBit 3 (2022) — UI profissional, dark mode
 *
 *  TÉCNICAS DEMONSTRADAS:
 *    1. Janela fullscreen sem borda (topmost)
 *    2. Bloqueio de Alt+F4 e teclas de escape
 *    3. Countdown timer via SetTimer (WM_TIMER)
 *    4. Renderização customizada com GDI (DrawText, FillRect)
 *    5. "Verificação de pagamento" (campo de input fake)
 *    6. Lista animada de arquivos sendo cifrados (WM_TIMER ID=3)
 *
 *  SAÍDA DE EMERGÊNCIA (LAB):
 *    Pressione ESC três vezes  OU  digite LABORATO + Enter.
 *
 *  COMPILE:
 *    g++ lockscreen.cpp -o lockscreen.exe -static -static-libgcc
 *        -static-libstdc++ -lgdi32 -luser32 -mwindows
 * ============================================================
 */

#include <windows.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <ctime>

// ═══════════════════════════════════════════════════════════
//  CONFIGURAÇÃO DO LAB
// ═══════════════════════════════════════════════════════════
const wchar_t* BTC_ADDRESS   = L"1LabXX9mK4rQ3fN8vT2uY7pW6hC5jD0eA";
const wchar_t* VALOR_RESGATE = L"0.05 BTC  (~R$ 15.000,00)";
const wchar_t* VICTIM_ID     = L"VICTIM-LAB-2025-A3F9B2C1";
const int      PRAZO_HORAS   = 72;
const wchar_t* CODIGO_FECHAR = L"LABORATO";

// Intervalo entre arquivos na animação (milissegundos)
// WannaCry cifrava ~1 arquivo a cada 300-500ms em média
#define INTERVALO_ARQUIVO_MS 400

// Quantos arquivos mostrar simultaneamente na lista visível
#define ARQUIVOS_VISIVEIS 12


// ═══════════════════════════════════════════════════════════
//  LISTA DE ARQUIVOS FICTÍCIOS
//
//  Simula uma máquina real sendo cifrada.
//  Caminhos realistas: Documents, Pictures, Desktop, Downloads,
//  D:\Backup, servidor de rede — exatamente o que WannaCry fazia.
//
//  ANÁLISE: Em análise dinâmica, o Process Monitor mostraria
//  cada um destes paths aparecendo em WriteFile + DeleteFile.
// ═══════════════════════════════════════════════════════════
const std::vector<std::wstring> ARQUIVOS_FALSOS = {
    L"C:\\Users\\Usuario\\Documents\\relatorio_financeiro_2025.xlsx",
    L"C:\\Users\\Usuario\\Documents\\contrato_cliente_ABC.docx",
    L"C:\\Users\\Usuario\\Documents\\proposta_comercial_v3.pdf",
    L"C:\\Users\\Usuario\\Documents\\planilha_orcamento_anual.xlsx",
    L"C:\\Users\\Usuario\\Documents\\apresentacao_diretoria.pptx",
    L"C:\\Users\\Usuario\\Documents\\notas_fiscais_marco_2025.zip",
    L"C:\\Users\\Usuario\\Documents\\backup_emails_2024.pst",
    L"C:\\Users\\Usuario\\Documents\\curriculo_atualizado.docx",
    L"C:\\Users\\Usuario\\Pictures\\ferias_familia_2024\\IMG_0001.jpg",
    L"C:\\Users\\Usuario\\Pictures\\ferias_familia_2024\\IMG_0002.jpg",
    L"C:\\Users\\Usuario\\Pictures\\ferias_familia_2024\\IMG_0047.jpg",
    L"C:\\Users\\Usuario\\Pictures\\formatura_pedro_2023.jpg",
    L"C:\\Users\\Usuario\\Pictures\\aniversario_ana_15anos.png",
    L"C:\\Users\\Usuario\\Pictures\\casamento_2022\\foto_cerimonia.jpg",
    L"C:\\Users\\Usuario\\Desktop\\projeto_tcc_final.docx",
    L"C:\\Users\\Usuario\\Desktop\\senhas_sistema.txt",
    L"C:\\Users\\Usuario\\Desktop\\acesso_banco_internet.pdf",
    L"C:\\Users\\Usuario\\Desktop\\chave_vpn_corporativa.key",
    L"C:\\Users\\Usuario\\Downloads\\extrato_bancario_fev2025.pdf",
    L"C:\\Users\\Usuario\\Downloads\\declaracao_imposto_renda_2024.pdf",
    L"C:\\Users\\Usuario\\Downloads\\certificado_digital.pfx",
    L"C:\\Users\\Usuario\\Downloads\\backup_whatsapp_2025.zip",
    L"C:\\Users\\Usuario\\Videos\\reuniao_gravada_jan2025.mp4",
    L"C:\\Users\\Usuario\\Videos\\treinamento_equipe_vendas.mp4",
    L"C:\\Users\\Usuario\\Music\\playlist_trabalho.mp3",
    L"D:\\Backup\\banco_dados_clientes.sql",
    L"D:\\Backup\\sistema_erp_dump_20250101.bak",
    L"D:\\Backup\\certificados_ssl\\wildcard_empresa.crt",
    L"D:\\Backup\\certificados_ssl\\chave_privada.pem",
    L"D:\\Backup\\configuracoes_servidor\\apache_httpd.conf",
    L"D:\\Backup\\configuracoes_servidor\\nginx.conf",
    L"D:\\Backup\\codigo_fonte\\projeto_web\\index.php",
    L"D:\\Backup\\codigo_fonte\\projeto_web\\config_bd.php",
    L"D:\\Backup\\codigo_fonte\\app_mobile\\MainActivity.java",
    L"D:\\Projetos\\sistema_gestao\\src\\database.cpp",
    L"D:\\Projetos\\sistema_gestao\\src\\auth_module.cpp",
    L"D:\\Projetos\\sistema_gestao\\docs\\manual_tecnico.pdf",
    L"D:\\RH\\funcionarios\\folha_pagamento_2025.xlsx",
    L"D:\\RH\\funcionarios\\dados_pessoais_completo.xlsx",
    L"D:\\RH\\contratos\\contrato_trabalho_maria_silva.pdf",
    L"D:\\Financeiro\\fluxo_caixa_2025.xlsx",
    L"D:\\Financeiro\\balancete_fevereiro_2025.pdf",
    L"D:\\Financeiro\\nota_fiscal_servicos_0042.xml",
    L"\\\\SERVIDOR01\\Compartilhado\\relatorios\\vendas_2025.xlsx",
    L"\\\\SERVIDOR01\\Compartilhado\\clientes\\base_leads.csv",
    L"\\\\SERVIDOR01\\Compartilhado\\marketing\\campanha_q1.psd",
    L"\\\\SERVIDOR01\\Backup\\snapshots\\vm_producao_2025.vmdk",
    L"\\\\SERVIDOR01\\Backup\\banco\\postgres_dump_20250129.tar.gz",
    L"C:\\inetpub\\wwwroot\\loja\\config\\database.php",
    L"C:\\inetpub\\wwwroot\\loja\\uploads\\produtos\\foto1.jpg",
    L"C:\\ProgramData\\MySQL\\data\\clientes_db\\pedidos.ibd",
    L"C:\\ProgramData\\MySQL\\data\\clientes_db\\usuarios.ibd",
    L"C:\\xampp\\htdocs\\sistema\\includes\\conexao.php",
    L"C:\\xampp\\htdocs\\sistema\\admin\\usuarios.php",
    L"C:\\Users\\Usuario\\AppData\\Roaming\\Thunderbird\\profile\\mail.sqlite",
    L"C:\\Users\\Usuario\\AppData\\Local\\Google\\Chrome\\User Data\\Default\\Cookies",
    L"C:\\Users\\Usuario\\AppData\\Roaming\\FileZilla\\sitemanager.xml",
    L"C:\\Users\\Administrador\\Documents\\chaves_acesso_aws.csv",
    L"C:\\Users\\Administrador\\Documents\\configuracoes_firewall.xlsx",
    L"D:\\Backup\\logs\\acesso_sistema_2025.log",
    L"D:\\Backup\\logs\\transacoes_financeiras.log",
    L"C:\\Users\\Usuario\\Documents\\contratos\\acordo_nda_empresa_xyz.pdf",
    L"C:\\Users\\Usuario\\Documents\\juridico\\parecer_advogado_2025.docx",
    L"D:\\Imagens\\produtos\\catalogo_2025\\produto_001.psd",
    L"D:\\Imagens\\produtos\\catalogo_2025\\produto_002.psd",
    L"\\\\NAS-BACKUP\\volumes\\vol1\\sistema\\backup_completo.tar",
    L"C:\\Users\\Usuario\\Documents\\pesquisa_desenvolvimento.docx",
    L"D:\\Segurança\\auditoria_acesso_2025.xlsx",
    L"C:\\Users\\Usuario\\Desktop\\codigo_acesso_cofre.txt",
};


// ═══════════════════════════════════════════════════════════
//  ESTADO GLOBAL DA APLICAÇÃO
// ═══════════════════════════════════════════════════════════
int          g_escCount          = 0;
int          g_segundosRestantes = 0;
std::wstring g_inputCodigo;
bool         g_codigoErrado      = false;
bool         g_codigoCorreto     = false;

// Estado da animação de arquivos
int  g_arquivoIdx    = 0;     // índice do próximo arquivo da lista
int  g_totalCifrados = 0;     // contador total de arquivos "cifrados"
std::vector<std::wstring> g_listaVisivel;  // últimos N arquivos exibidos

void inicializarTimer() {
    g_segundosRestantes = PRAZO_HORAS * 3600;
}

std::wstring formatarTempo(int segundos) {
    int h = segundos / 3600;
    int m = (segundos % 3600) / 60;
    int s = segundos % 60;
    std::wostringstream ss;
    ss << std::setfill(L'0')
       << std::setw(2) << h << L":"
       << std::setw(2) << m << L":"
       << std::setw(2) << s;
    return ss.str();
}

// Avança para o próximo arquivo fictício na animação
void avancarArquivo() {
    if (ARQUIVOS_FALSOS.empty()) return;

    std::wstring arq = ARQUIVOS_FALSOS[g_arquivoIdx % ARQUIVOS_FALSOS.size()];
    g_arquivoIdx++;
    g_totalCifrados++;

    g_listaVisivel.push_back(arq);
    if ((int)g_listaVisivel.size() > ARQUIVOS_VISIVEIS)
        g_listaVisivel.erase(g_listaVisivel.begin());
}

// Formata o total cifrado com separador de milhar
std::wstring formatarContador(int n) {
    std::wstring s = std::to_wstring(n);
    int pos = (int)s.size() - 3;
    while (pos > 0) { s.insert(pos, L"."); pos -= 3; }
    return s;
}


// ═══════════════════════════════════════════════════════════
//  CORES
// ═══════════════════════════════════════════════════════════
#define COR_FUNDO         RGB( 20,  20,  20)
#define COR_PAINEL        RGB(180,  20,  20)
#define COR_TEXTO_BRANCO  RGB(255, 255, 255)
#define COR_TEXTO_AMARELO RGB(255, 200,   0)
#define COR_TEXTO_CINZA   RGB(180, 180, 180)
#define COR_TIMER         RGB(255,  60,  60)
#define COR_BTC           RGB(247, 147,  26)
#define COR_VERDE         RGB(  0, 220,  80)
#define COR_ARQUIVO_NOVO  RGB(255, 255, 100)   // Amarelo vivo — arquivo atual
#define COR_ARQUIVO_OK    RGB(160, 255, 160)   // Verde claro — já cifrado
#define COR_ARQUIVO_VELHO RGB(120, 120, 120)   // Cinza — mais antigos


// ═══════════════════════════════════════════════════════════
//  PAINEL ESQUERDO — redesenhado com lista de arquivos
// ═══════════════════════════════════════════════════════════
void desenharPainelEsquerdo(HDC hdc, int W, int H) {
    int xE = 20;
    int wE = (int)(W * 0.62) - 40;

    // Fundo vermelho
    RECT painelEsq = { 0, 0, (LONG)(W * 0.62), H };
    HBRUSH brPainel = CreateSolidBrush(COR_PAINEL);
    FillRect(hdc, &painelEsq, brPainel);
    DeleteObject(brPainel);

    SetBkMode(hdc, TRANSPARENT);

    // ── Título ────────────────────────────────────────────
    HFONT fntTitulo = CreateFontW(42, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(hdc, fntTitulo);
    SetTextColor(hdc, COR_TEXTO_BRANCO);
    RECT rTitulo = { xE, 12, xE + wE, 65 };
    DrawTextW(hdc, L"[  !  ]  Oops, seus arquivos foram cifrados!", -1,
              &rTitulo, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(fntTitulo);

    // Linha separadora
    HPEN penLinha = CreatePen(PS_SOLID, 2, COR_TEXTO_BRANCO);
    SelectObject(hdc, penLinha);
    MoveToEx(hdc, xE, 70, NULL);
    LineTo(hdc, xE + wE, 70);
    DeleteObject(penLinha);

    // ── Texto explicativo (comprimido) ────────────────────
    HFONT fntTexto = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(hdc, fntTexto);
    SetTextColor(hdc, COR_TEXTO_BRANCO);
    const wchar_t* textoExpl =
        L"Seus arquivos foram cifrados com AES-256 + RSA-2048. "
        L"Sem a chave privada em nosso servidor, a recuperacao e impossivel. "
        L"Pague o resgate antes do prazo — apos 7 dias a chave e destruida.";
    RECT rTexto = { xE, 78, xE + wE, 130 };
    DrawTextW(hdc, textoExpl, -1, &rTexto, DT_LEFT | DT_WORDBREAK);
    DeleteObject(fntTexto);

    // ── Seção: arquivos sendo cifrados ────────────────────
    // Fundo levemente mais escuro para destacar a lista
    RECT rListaBg = { 0, 135, (LONG)(W * 0.62), H - 120 };
    HBRUSH brLista = CreateSolidBrush(RGB(140, 15, 15));
    FillRect(hdc, &rListaBg, brLista);
    DeleteObject(brLista);

    // Título da seção + contador
    HFONT fntSecLabel = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(hdc, fntSecLabel);
    SetTextColor(hdc, COR_TEXTO_AMARELO);

    std::wstring labelArqs = L"  Arquivos sendo criptografados:";
    RECT rSecLabel = { xE, 140, xE + wE / 2, 160 };
    DrawTextW(hdc, labelArqs.c_str(), -1, &rSecLabel, DT_LEFT);

    // Contador à direita do título
    std::wstring contador = formatarContador(g_totalCifrados) + L" arquivos cifrados";
    SetTextColor(hdc, COR_VERDE);
    RECT rContador = { xE + wE / 2, 140, xE + wE, 160 };
    DrawTextW(hdc, contador.c_str(), -1, &rContador, DT_RIGHT);
    DeleteObject(fntSecLabel);

    // Linha fina abaixo do label
    HPEN penFino = CreatePen(PS_SOLID, 1, RGB(200, 60, 60));
    SelectObject(hdc, penFino);
    MoveToEx(hdc, xE, 162, NULL);
    LineTo(hdc, xE + wE, 162);
    DeleteObject(penFino);

    // ── Lista de arquivos animada ─────────────────────────
    //
    // TÉCNICA DE ANÁLISE:
    // Em Process Monitor, cada arquivo desta lista geraria:
    //   CreateFile (leitura)  → ReadFile  → WriteFile (.ENCLAB)
    //   → DeleteFile (original removido)
    // O analista vê este padrão e identifica o ransomware imediatamente.
    //
    HFONT fntArquivo = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Courier New");
    SelectObject(hdc, fntArquivo);

    int yArq = 168;
    int alturaLinha = 22;
    int tamanho = (int)g_listaVisivel.size();

    for (int i = 0; i < tamanho; i++) {
        int posRelativa = tamanho - 1 - i; // 0 = mais recente

        // Cor e símbolo variam conforme a posição:
        // - mais recente: amarelo vivo + ► (cifrando agora)
        // - recentes: verde claro + ✓
        // - antigos: cinza + ✓
        std::wstring prefixo;
        COLORREF cor;

        if (posRelativa == 0) {
            prefixo = L" >> ";
            cor = COR_ARQUIVO_NOVO;
        } else if (posRelativa <= 3) {
            prefixo = L" OK ";
            cor = COR_ARQUIVO_OK;
        } else {
            prefixo = L" OK ";
            cor = COR_ARQUIVO_VELHO;
        }

        SetTextColor(hdc, cor);
        std::wstring linha = prefixo + g_listaVisivel[i];

        RECT rArq = { xE, yArq + i * alturaLinha,
                      xE + wE, yArq + (i + 1) * alturaLinha };
        DrawTextW(hdc, linha.c_str(), -1, &rArq,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    // Se ainda não há arquivos, mostra animação de "iniciando"
    if (tamanho == 0) {
        SetTextColor(hdc, COR_TEXTO_CINZA);
        RECT rAguarda = { xE, yArq, xE + wE, yArq + 30 };
        DrawTextW(hdc, L" Iniciando processo de cifragem...", -1,
                  &rAguarda, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    DeleteObject(fntArquivo);

    // ── Timer countdown ───────────────────────────────────
    HFONT fntTimerLabel = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(hdc, fntTimerLabel);
    SetTextColor(hdc, COR_TEXTO_AMARELO);
    RECT rTLabel = { xE, H - 115, xE + wE, H - 95 };
    DrawTextW(hdc, L"Tempo restante para o valor dobrar:", -1, &rTLabel, DT_LEFT);
    DeleteObject(fntTimerLabel);

    HFONT fntTimer = CreateFontW(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Courier New");
    SelectObject(hdc, fntTimer);
    SetTextColor(hdc, COR_TIMER);
    std::wstring tempoStr = formatarTempo(g_segundosRestantes);
    RECT rTimer = { xE, H - 95, xE + wE, H - 45 };
    DrawTextW(hdc, tempoStr.c_str(), -1, &rTimer, DT_CENTER);
    DeleteObject(fntTimer);

    // ── ID da vítima + rodapé ─────────────────────────────
    HFONT fntId = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Courier New");
    SelectObject(hdc, fntId);
    SetTextColor(hdc, RGB(220, 180, 180));
    RECT rId = { xE, H - 42, xE + wE, H - 24 };
    std::wstring idTxt = std::wstring(L"ID: ") + VICTIM_ID;
    DrawTextW(hdc, idTxt.c_str(), -1, &rId, DT_LEFT);

    SetTextColor(hdc, RGB(200, 150, 150));
    RECT rRodape = { xE, H - 22, xE + wE, H - 4 };
    DrawTextW(hdc, L"[LAB] ESC x3 para sair  |  Curso: Defesa Cibernetica",
              -1, &rRodape, DT_CENTER);
    DeleteObject(fntId);
}


// ═══════════════════════════════════════════════════════════
//  PAINEL DIREITO — instruções de pagamento
// ═══════════════════════════════════════════════════════════
void desenharPainelDireito(HDC hdc, int W, int H) {
    RECT painelDir = { (LONG)(W * 0.62), 0, W, H };
    HBRUSH brDir = CreateSolidBrush(RGB(30, 30, 30));
    FillRect(hdc, &painelDir, brDir);
    DeleteObject(brDir);

    int xD = (int)(W * 0.62) + 20;
    SetBkMode(hdc, TRANSPARENT);

    // Título
    HFONT fntTit = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(hdc, fntTit);
    SetTextColor(hdc, COR_TEXTO_BRANCO);
    RECT rTit = { xD, 20, W - 20, 58 };
    DrawTextW(hdc, L"Como pagar o resgate", -1, &rTit, DT_LEFT);
    DeleteObject(fntTit);

    HPEN penSep = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
    SelectObject(hdc, penSep);
    MoveToEx(hdc, xD, 62, NULL);
    LineTo(hdc, W - 20, 62);
    DeleteObject(penSep);

    // Passos
    HFONT fntP = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(hdc, fntP);

    struct { int y; COLORREF cor; const wchar_t* txt; } passos[] = {
        { 75,  COR_TEXTO_AMARELO, L"Passo 1 — Obtenha Bitcoin" },
        { 95,  COR_TEXTO_CINZA,   L"Use Binance ou Mercado Bitcoin." },
        { 130, COR_TEXTO_AMARELO, L"Passo 2 — Envie o pagamento" },
        { 150, COR_TEXTO_CINZA,   L"Envie exatamente:" },
        { 188, COR_TEXTO_AMARELO, L"Passo 3 — Nos contate" },
        { 208, COR_TEXTO_CINZA,   L"Email: lab@educacional.onion" },
        { 226, COR_TEXTO_CINZA,   L"Informe seu ID ao contatar." },
    };
    for (auto& p : passos) {
        SetTextColor(hdc, p.cor);
        RECT r = { xD, p.y, W - 20, p.y + 28 };
        DrawTextW(hdc, p.txt, -1, &r, DT_LEFT);
    }
    DeleteObject(fntP);

    // Valor do resgate
    HFONT fntVal = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(hdc, fntVal);
    SetTextColor(hdc, COR_TEXTO_BRANCO);
    RECT rVal = { xD, 152, W - 20, 188 };
    DrawTextW(hdc, VALOR_RESGATE, -1, &rVal, DT_LEFT);
    DeleteObject(fntVal);

    // Endereço Bitcoin
    RECT rBtcBg = { xD - 5, 256, W - 15, 296 };
    HBRUSH brBtc = CreateSolidBrush(RGB(40, 30, 10));
    FillRect(hdc, &rBtcBg, brBtc);
    DeleteObject(brBtc);

    HFONT fntBtc = CreateFontW(13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Courier New");
    SelectObject(hdc, fntBtc);
    SetTextColor(hdc, COR_BTC);
    RECT rBtcLbl = { xD, 244, W - 20, 260 };
    DrawTextW(hdc, L"Endereco Bitcoin:", -1, &rBtcLbl, DT_LEFT);
    RECT rBtc = { xD, 260, W - 20, 296 };
    DrawTextW(hdc, BTC_ADDRESS, -1, &rBtc, DT_LEFT);
    DeleteObject(fntBtc);

    // Campo de desbloqueio
    HFONT fntCL = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(hdc, fntCL);
    SetTextColor(hdc, COR_TEXTO_CINZA);
    RECT rCL = { xD, 318, W - 20, 340 };
    DrawTextW(hdc, L"Ja pagou? Digite o codigo de desbloqueio:", -1, &rCL, DT_LEFT);
    DeleteObject(fntCL);

    RECT rCaixaBg = { xD - 5, 343, W - 78, 373 };
    HBRUSH brCaixa = CreateSolidBrush(RGB(50, 50, 50));
    FillRect(hdc, &rCaixaBg, brCaixa);
    DeleteObject(brCaixa);

    HPEN penCaixa;
    if (g_codigoErrado)       penCaixa = CreatePen(PS_SOLID, 2, COR_TIMER);
    else if (g_codigoCorreto) penCaixa = CreatePen(PS_SOLID, 2, COR_VERDE);
    else                      penCaixa = CreatePen(PS_SOLID, 1, RGB(120, 120, 120));
    SelectObject(hdc, penCaixa);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, rCaixaBg.left, rCaixaBg.top, rCaixaBg.right, rCaixaBg.bottom);
    DeleteObject(penCaixa);

    HFONT fntInput = CreateFontW(17, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Courier New");
    SelectObject(hdc, fntInput);

    if (g_codigoCorreto) {
        SetTextColor(hdc, COR_VERDE);
        RECT r = { xD, 343, W - 20, 373 };
        DrawTextW(hdc, L"  CODIGO CORRETO — Encerrando...", -1, &r,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    } else {
        SetTextColor(hdc, g_codigoErrado ? COR_TIMER : COR_TEXTO_BRANCO);
        std::wstring txt = L"  " + g_inputCodigo + L"_";
        RECT r = { xD, 343, W - 78, 373 };
        DrawTextW(hdc, txt.c_str(), -1, &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    DeleteObject(fntInput);

    if (g_codigoErrado) {
        HFONT fntErr = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        SelectObject(hdc, fntErr);
        SetTextColor(hdc, COR_TIMER);
        RECT r = { xD, 377, W - 20, 395 };
        DrawTextW(hdc, L"Codigo invalido. Tente novamente.", -1, &r, DT_LEFT);
        DeleteObject(fntErr);
    }

    // Botão OK
    RECT rBtn = { W - 74, 343, W - 16, 373 };
    HBRUSH brBtn = CreateSolidBrush(COR_PAINEL);
    FillRect(hdc, &rBtn, brBtn);
    DeleteObject(brBtn);
    HFONT fntBtn = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(hdc, fntBtn);
    SetTextColor(hdc, COR_TEXTO_BRANCO);
    DrawTextW(hdc, L"OK", -1, &rBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(fntBtn);

    // Nota técnica
    HFONT fntNota = CreateFontW(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(hdc, fntNota);
    SetTextColor(hdc, RGB(70, 70, 70));
    RECT rNota = { xD, H - 75, W - 20, H - 10 };
    DrawTextW(hdc,
        L"[ANALISE] Codigo = LABORATO\r\n"
        L"Em ataques reais, so o C2 tem a chave.\r\n"
        L"Sem pagamento = recuperacao impossivel.",
        -1, &rNota, DT_LEFT | DT_WORDBREAK);
    DeleteObject(fntNota);
}


// ═══════════════════════════════════════════════════════════
//  DESENHO COMPLETO DA TELA
// ═══════════════════════════════════════════════════════════
void desenharTela(HDC hdc, RECT clienteRect) {
    int W = clienteRect.right;
    int H = clienteRect.bottom;

    // Fundo preto base
    HBRUSH brFundo = CreateSolidBrush(COR_FUNDO);
    FillRect(hdc, &clienteRect, brFundo);
    DeleteObject(brFundo);

    SetBkMode(hdc, TRANSPARENT);
    desenharPainelEsquerdo(hdc, W, H);
    desenharPainelDireito(hdc, W, H);
}


// ═══════════════════════════════════════════════════════════
//  WINDOW PROCEDURE
// ═══════════════════════════════════════════════════════════
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_TIMER:
        if (wParam == 1) {
            // Timer 1 (1000ms): countdown
            if (g_segundosRestantes > 0) g_segundosRestantes--;
            InvalidateRect(hWnd, NULL, FALSE);
        } else if (wParam == 2) {
            // Timer 2 (1500ms): fecha após código correto
            PostQuitMessage(0);
        } else if (wParam == 3) {
            // Timer 3 (400ms): avança lista de arquivos cifrados
            avancarArquivo();
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;

    case WM_KEYDOWN: {
        int vk = (int)wParam;

        if (vk == VK_ESCAPE) {
            g_escCount++;
            if (g_escCount >= 3) PostQuitMessage(0);
            return 0;
        }
        g_escCount = 0;

        if (vk == VK_F4 && (GetKeyState(VK_MENU) & 0x8000)) return 0;

        if (vk == VK_BACK && !g_inputCodigo.empty()) {
            g_inputCodigo.pop_back();
            g_codigoErrado = false;
        }

        if (vk == VK_RETURN) {
            if (g_inputCodigo == CODIGO_FECHAR) {
                g_codigoCorreto = true;
                KillTimer(hWnd, 3); // Para a animação de arquivos
                InvalidateRect(hWnd, NULL, FALSE);
                SetTimer(hWnd, 2, 1500, NULL);
            } else {
                g_codigoErrado = true;
            }
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    case WM_CHAR: {
        wchar_t c = (wchar_t)wParam;
        if (c >= 32 && c < 127 && g_inputCodigo.size() < 20) {
            g_inputCodigo += towupper(c);
            g_codigoErrado = false;
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rect;
        GetClientRect(hWnd, &rect);

        // Double buffering — sem flickering
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
        SelectObject(hdcMem, bmp);

        desenharTela(hdcMem, rect);

        BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
        DeleteObject(bmp);
        DeleteDC(hdcMem);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_CLOSE) return 0;
        break;

    case WM_CLOSE:
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}


// ═══════════════════════════════════════════════════════════
//  WinMain
// ═══════════════════════════════════════════════════════════
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {

    inicializarTimer();

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"RansomLock";
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExW(&wc);

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    HWND hWnd = CreateWindowExW(
        WS_EX_TOPMOST,
        L"RansomLock", L"",
        WS_POPUP | WS_VISIBLE,
        0, 0, sw, sh,
        NULL, NULL, hInst, NULL
    );

    SetTimer(hWnd, 1, 1000,                  NULL); // countdown (1s)
    SetTimer(hWnd, 3, INTERVALO_ARQUIVO_MS,  NULL); // animação arquivos (400ms)

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
