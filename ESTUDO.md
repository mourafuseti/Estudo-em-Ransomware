# Guia de Estudo — Malware Analysis: Ransomware em C++

## Arquivos do Lab

| Arquivo | Função |
|---|---|
| `setup_lab.cpp` | Cria pasta de teste com arquivos fictícios |
| `ransomware_sim.cpp` | Simula o ransomware (só age em C:\RansomwareTest) |
| `decryptor.cpp` | Recupera os arquivos — simula trabalho de IR |

---

## Como Compilar (MinGW / g++)

```bash
g++ setup_lab.cpp    -o setup_lab.exe
g++ ransomware_sim.cpp -o ransomware_sim.exe -lkernel32
g++ decryptor.cpp    -o decryptor.exe    -lkernel32
```

## Ordem de Execução

```
1. setup_lab.exe       → cria C:\RansomwareTest com arquivos fictícios
2. ransomware_sim.exe  → cifra os arquivos (veja o resultado!)
3. decryptor.exe       → recupera os arquivos
```

---

## Técnicas Demonstradas

### 1. Enumeração de Arquivos
- **API**: `FindFirstFileA` + `FindNextFileA`
- **IOC**: Presença na IAT (Import Address Table) do binário
- **Detecção**: Process Monitor — filtro `Operation = ReadFile` + alto volume

### 2. Cifragem XOR
- **Real**: Ransomwares usam AES-256 + RSA-2048
- **Detecção de API suspeita**: `CryptEncrypt`, `BCryptEncrypt`
- **Fraqueza didática**: chave hardcoded, facilmente extraída com `strings`

### 3. Magic Bytes / Cabeçalho
- **Uso**: Identificar arquivos cifrados por esta família
- **YARA Rule example**:
  ```yara
  rule RansomwareLab {
      strings:
          $magic = "ENCLAB_V1"
      condition:
          $magic at 0
  }
  ```

### 4. Exclusão do Original
- **API**: `DeleteFileA`
- **IOC Forte**: CreateFile (write) seguido de DeleteFile no mesmo diretório

### 5. Ransom Note
- **Artefato forense**: Estilo, wallet, contato → atribuição
- **Localização**: Criada em CADA subdiretório visitado

---

## Ferramentas de Análise para Praticar

| Ferramenta | O que analisar |
|---|---|
| `strings ransomware_sim.exe` | Extrai literais: extensão, magic, paths |
| Process Monitor (Sysinternals) | ReadFile, WriteFile, DeleteFile em tempo real |
| x64dbg | Breakpoint em `FindFirstFileA`, inspeciona buffers XOR |
| Ghidra / IDA Free | Engenharia reversa do binário compilado |
| YARA | Escreva regras baseadas no magic header |

---

## Perguntas de Estudo

1. Como você extrairia a chave XOR de `ransomware_sim.exe` usando x64dbg sem ver o código-fonte?
2. Por que ransomwares reais usam RSA + AES em vez de só AES?
3. O que é "double extortion" e como isso mudou a estratégia de IR?
4. Como a função `enumerarECifrar` poderia ser modificada para pular pastas de sistema?
5. Escreva uma YARA rule para detectar arquivos cifrados por este simulador.
