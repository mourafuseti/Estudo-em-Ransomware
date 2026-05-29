# Estudo em Ransomware — Malware Analysis em C++

> **Finalidade exclusivamente educacional** — Desenvolvido como material de estudo para o curso de **Defesa Cibernética / Malware Analysis**. Todo código opera em ambiente controlado e nunca deve ser executado fora de uma VM isolada ou da pasta de laboratório designada.

---

## Sobre o Projeto

Este repositório replica arquitetura e técnicas de ransomwares reais (WannaCry, LockBit, Stuxnet) em C++ com WinAPI pura, com comentários didáticos explicando cada técnica do ponto de vista do **analista de defesa**.

Cada arquivo é acompanhado de:
- Explicação da técnica usada
- Como analistas detectam via análise estática e dinâmica
- IOCs (Indicators of Compromise) gerados
- Ferramentas para detectar e mitigar

---

## Arquivos do Lab

| Arquivo | Descrição | Técnicas |
|---|---|---|
| `setup_lab.cpp` | Cria ambiente de teste em `C:\RansomwareTest\` | Criação de estrutura de diretórios |
| `ransomware_sim.cpp` | Simula cifragem de arquivos | FindFirstFile, XOR cipher, DeleteFile, Ransom Note |
| `decryptor.cpp` | Recupera arquivos cifrados | Magic header validation, XOR decrypt |
| `lockscreen.cpp` | Tela de resgate fullscreen (estilo WannaCry) | WinAPI GDI, WS_EX_TOPMOST, WM_SYSCOMMAND block, countdown timer |
| `usb_worm.cpp` | Propagação via pendrive | WM_DEVICECHANGE, autorun.inf, .lnk malicioso, atributos Hidden+System |

---

## Técnicas Demonstradas

### Ransomware Core
- **Enumeração recursiva** — `FindFirstFileA` / `FindNextFileA`
- **Cifragem XOR** — keystream cíclico com marcador de cabeçalho (magic bytes)
- **Exclusão do original** — `DeleteFileA` após cifrar (IOC forte)
- **Nota de resgate** — criada em cada subdiretório visitado

### Lock Screen (WannaCry-style)
- **Janela fullscreen sem borda** — `WS_POPUP | WS_EX_TOPMOST`
- **Bloqueio de Alt+F4** — `WM_SYSCOMMAND SC_CLOSE` ignorado
- **Countdown timer** — `SetTimer` + `WM_TIMER`
- **Double buffering GDI** — sem flickering na atualização
- **Campo de código** — verificação de "pagamento"

### USB Worm (Stuxnet-inspired)
- **Detecção de mídia removível** — `GetDriveTypeA` + `GetVolumeInformationA`
- **Auto-cópia** — `GetModuleFileName` + `CopyFileA`
- **autorun.inf** — técnica clássica (Windows XP/Vista)
- **Atalho .lnk malicioso** — `IShellLink` COM, ícone de pasta, execução oculta
- **Ocultação** — `SetFileAttributesA` com `FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM`
- **Monitoramento de eventos** — janela oculta com `WM_DEVICECHANGE`

---

## Como Compilar

Requer **MSYS2 + MinGW-w64** (`pacman -S mingw-w64-ucrt-x86_64-gcc`).

```bash
# Setup do ambiente
g++ setup_lab.cpp    -o setup_lab.exe    -static -static-libgcc -static-libstdc++

# Ransomware simulado
g++ ransomware_sim.cpp -o ransomware_sim.exe -static -static-libgcc -static-libstdc++

# Decryptor
g++ decryptor.cpp    -o decryptor.exe    -static -static-libgcc -static-libstdc++

# Lock screen (WannaCry-style)
g++ lockscreen.cpp   -o lockscreen.exe   -static -static-libgcc -static-libstdc++ -lgdi32 -luser32 -mwindows

# USB Worm
g++ usb_worm.cpp     -o usb_worm.exe     -static -static-libgcc -static-libstdc++ -lole32 -lshell32 -luuid
```

---

## Ordem de Execução do Lab

```
1. setup_lab.exe        → cria C:\RansomwareTest\ com arquivos fictícios
2. ransomware_sim.exe   → cifra os arquivos (observe os .ENCLAB criados)
3. [analise]            → abra C:\RansomwareTest\ no Explorer
4. decryptor.exe        → recupera todos os arquivos
5. lockscreen.exe       → tela de resgate fullscreen (ESC x3 para sair)
6. usb_worm.exe         → insira um pendrive e observe a propagação
```

---

## Ferramentas de Análise Recomendadas

| Ferramenta | Uso no Lab |
|---|---|
| **Process Monitor** (Sysinternals) | Captura ReadFile → WriteFile → DeleteFile em tempo real |
| **Autoruns** (Sysinternals) | Aba *Removable Media* — detecta autorun.inf |
| **x64dbg** | Breakpoint em `FindFirstFileA`, extrai chave XOR do buffer |
| **Ghidra / IDA Free** | Engenharia reversa dos binários compilados |
| **strings** (binutils) | `strings ransomware_sim.exe` extrai literais, extensão, magic bytes |
| **Wireshark** | Monitora tráfego de rede gerado pelo malware |

---

## Detecção e IOCs

### Análise Estática
- Strings: `.ENCLAB`, `ENCLAB_V1`, `LEIA_ESTE_ARQUIVO`, `autorun.inf`
- Imports na IAT: `FindFirstFileA`, `CopyFileA`, `DeleteFileA`, `SetFileAttributesA`
- Linkagem estática (executável grande sem dependências externas)

### Análise Dinâmica (Process Monitor)
```
Operation=WriteFile  + Path ends with .ENCLAB     → cifragem ativa
Operation=DeleteFile + Path ends with .txt/.doc   → exclusão do original
Operation=WriteFile  + Path=autorun.inf           → USB worm
Operation=CreateFile + Path=*.lnk                 → atalho malicioso
```

### YARA Rule (exemplo)
```yara
rule RansomwareLab {
    strings:
        $magic   = "ENCLAB_V1"
        $nota    = "LEIA_ESTE_ARQUIVO"
        $ext     = ".ENCLAB"
    condition:
        any of them
}
```

---

## Saídas de Emergência (Lab)

| Programa | Como fechar |
|---|---|
| `lockscreen.exe` | Pressionar **ESC três vezes** ou digitar `LABORATO` + Enter |
| `usb_worm.exe` | Responder `n` na confirmação ou **Ctrl+C** |
| `ransomware_sim.exe` | Não possui — executa e termina sozinho |

---

## Contexto Histórico

| Malware | Ano | Técnica principal | Impacto |
|---|---|---|---|
| **Conficker** | 2008 | USB autorun.inf + SMB | 15 milhões de máquinas |
| **Agent.BTZ** | 2008 | USB drop em base militar | Redes do Pentágono comprometidas |
| **Stuxnet** | 2010 | LNK exploit + USB | Usinas nucleares iranianas |
| **WannaCry** | 2017 | EternalBlue + lock screen | 230.000 máquinas, NHS parado |
| **LockBit 3.0** | 2022 | Ransomware-as-a-Service | Maior operação de ransomware ativo |

---

## Perguntas de Estudo

1. Como extrair a chave XOR de `ransomware_sim.exe` usando x64dbg sem ver o código-fonte?
2. Por que ransomwares reais usam AES-256 + RSA-2048 em vez de só AES?
3. O que é *double extortion* e como isso mudou a estratégia de IR?
4. Como o Stuxnet executava seu payload sem nenhum clique do usuário (CVE-2010-2568)?
5. Escreva uma YARA rule para detectar arquivos cifrados por este simulador.
6. Como um USB Write Blocker impede a propagação do `usb_worm.exe`?
7. Por que `WM_DEVICECHANGE` é superior a polling para detecção de USB?

---

## Aviso Legal

Este material é produzido exclusivamente para fins educacionais no contexto de cursos de segurança ofensiva e defensiva. O uso destas técnicas contra sistemas sem autorização explícita é crime tipificado na **Lei nº 12.737/2012 (Lei Carolina Dieckmann)** e no **Marco Civil da Internet (Lei nº 12.965/2014)**.

---

*Curso: Defesa Cibernética — Malware Analysis em C++*
