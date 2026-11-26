# DebugManager

Log em **6 níveis** para Arduino e ESP32, com **saídas extras** (arquivo, MQTT, display...), **gravação no LittleFS** e API em **português e inglês**.

> 📖 **Documentação completa / Full documentation (PT/EN):** https://professorthiago.github.io/DebugManager/
>
> Autor: [professorThiago](https://github.com/professorThiago) · Licença MIT

```cpp
#include <DebugManager.h>
#include <DebugLittleFS.h>   // opcional (ESP32): grava o log em arquivo

void setup() {
    configurarDebug(DEBUG_INFO, 4);       // Serial até INFO; GPIO 4 no GND ao ligar → TUDO
    debugLittleFS.iniciar(DEBUG_AVISO);   // avisos e erros também em /logs/atual.log
    debugInfo("Sistema pronto");
    debugInfof("Temperatura: %.1f C", 23.4);
}

void loop() {
    debugLittleFS.atualizar();
}
```

---

## Sumário

- [Níveis](#níveis)
- [Instalação](#instalação)
- [Uso básico](#uso-básico)
- [Saídas extras](#saídas-extras)
- [Log no LittleFS (ESP32)](#log-no-littlefs-esp32)
- [English API](#english-api)
- [Referência](#referência)
- [Exemplos](#exemplos)
- [Histórico de versões](#histórico-de-versões)

---

## Níveis

Cada nível inclui todos os anteriores.

| Nível | PT | EN | Prefixo | Quando usar |
|:---:|:---|:---|:---|:---|
| 0 | `DEBUG_NENHUM` | `DEBUG_NONE` | — | Produção silenciosa |
| 1 | `DEBUG_ERRO` | `DEBUG_ERROR` | `[ERRO]` | Falhas que afetam o funcionamento |
| 2 | `DEBUG_AVISO` | `DEBUG_WARNING` | `[AVISO]` | Reconexões, timeouts, fila cheia |
| 3 | `DEBUG_INFO` | `DEBUG_INFO` | `[INFO]` | Fluxo normal (conectou, publicou) |
| 4 | `DEBUG_VERBOSE` | `DEBUG_VERBOSE` | `[VERBOSE]` | Tentativas, transições de estado |
| 5 | `DEBUG_TUDO` | `DEBUG_TRACE` | `[TUDO]` | Payloads brutos, códigos numéricos |

---

## Instalação

```ini
; platformio.ini
lib_deps =
    https://github.com/professorThiago/DebugManager.git
```

Para usar o log em arquivo, acrescente `board_build.filesystem = littlefs`.

Arduino IDE: baixe o ZIP e use **Sketch → Incluir Biblioteca → Adicionar biblioteca .ZIP**.

---

## Uso básico

```cpp
configurarDebug(DEBUG_INFO);          // nível do Serial
configurarDebug(DEBUG_INFO, 4);       // GPIO 4 no GND ao ligar força DEBUG_TUDO
configurarDebug(DEBUG_INFO, -1, 0);   // 0 = não chama Serial.begin() (seu código já chamou)

debugErro("Sensor nao respondeu");                  // texto fixo: não cria String
debugAviso("Fila em " + String(fila) + "/30");      // String
debugInfof("RSSI: %d dBm | heap: %u", rssi, heap);  // estilo printf

definirNivelDebug(DEBUG_VERBOSE);     // muda em tempo de execução
mostrarTempoDebug(true);              // [   12.345] [INFO]    ...
definirPortaDebug(Serial1);           // outra porta serial

// Evita montar textos caros que seriam descartados
if (debugHabilitado(DEBUG_TUDO)) debugTudo("Payload: " + payloadGigante);
```

---

## Saídas extras

Uma saída extra recebe as mensagens para fazer o que você quiser — gravar em arquivo, publicar por MQTT, mostrar num display. **Cada saída tem o seu nível**, independente do Serial.

```cpp
int id = adicionarSaidaDebug([](int nivel, const char* msg) {
    // chamada para cada mensagem com nível <= DEBUG_AVISO
    display.println(msg);
}, DEBUG_AVISO);

definirNivelSaidaDebug(id, DEBUG_ERRO);   // muda o nível dessa saída
removerSaidaDebug(id);                    // remove
```

- Até 4 saídas ao mesmo tempo (`-DDEBUG_MANAGER_MAX_SAIDAS=8` para mais).
- Uma mensagem gerada **dentro** de uma saída extra vai só para o Serial (evita loop infinito).
- No ESP32, as chamadas são protegidas por mutex: várias tarefas podem logar sem misturar linhas.
- No ESP32/ESP8266/RP2040 aceita lambdas com captura; no AVR, funções comuns ou lambdas sem captura.

---

## Log no LittleFS (ESP32)

`DebugLittleFS` é uma saída extra pronta que grava em arquivo. Tudo o que passa pelo DebugManager — **inclusive as mensagens das bibliotecas que o usam, como a ConectividadeESP32** — vai para `/logs/atual.log` com data e hora.

```cpp
#include <DebugManager.h>
#include <DebugLittleFS.h>

void setup() {
    configurarDebug(DEBUG_INFO);

    DebugLittleFS::Config cfg;
    cfg.nivel             = DEBUG_AVISO;   // o que vai para o arquivo
    cfg.tamanhoMaxArquivo = 64 * 1024;     // gira a cada 64 KB
    cfg.arquivosAntigos   = 2;             // atual.log + 1.log + 2.log
    debugLittleFS.iniciar(cfg);
}

void loop() {
    debugLittleFS.atualizar();             // grava o buffer e traz os erros do ESP-IDF
}

// Quando o WiFi conectar: data e hora reais no log (padrão: horário de Brasília)
debugLittleFS.sincronizarHorario();
```

Exemplo de arquivo:

```
boot+00:00:00.412   I ==== BOOT | reset: BROWNOUT (queda de tensao) | heap: 251320 ====
boot+00:00:16.903   W WiFi: timeout (15s). Falha 1.
2026-11-26 14:03:12 E MQTT: falha na conexao! Codigo: -2: CONNECT_FAILED
2026-11-26 14:03:12 E IDF: (16210) esp-tls: ...
```

Letras: `E` erro · `W` aviso · `I` info · `V` verbose · `T` tudo.

| Função | Descrição |
|:---|:---|
| `iniciar(cfg)` / `iniciar(nivel)` | Monta o LittleFS e começa a gravar |
| `atualizar()` | No `loop()`: grava o buffer e os erros do sistema |
| `gravarAgora()` | Grava já (use antes de `ESP.restart()`) |
| `definirNivel(n)` | Muda o nível do arquivo |
| `sincronizarHorario()` | NTP — chame quando o WiFi conectar |
| `ultimasLinhas(n)` | Últimas N linhas (texto) |
| `paraCadaLinha(n, cb)` | Percorre as últimas N linhas (ex.: enviar por MQTT em partes) |
| `imprimirTudo(Serial)` | Todos os arquivos, do mais antigo ao atual |
| `limpar()` / `tamanhoTotal()` | Apaga / bytes ocupados |
| `DebugLittleFS::motivoReset()` | Motivo do último reset (energia, watchdog, pânico...) |

**Cuidados com a flash:**
- Linhas comuns ficam em um buffer na RAM e vão para a flash a cada 30 s (ou ao passar de 2 KB).
- **Erros e avisos são gravados na hora** — se o ESP32 travar, eles já estão salvos.
- O LittleFS **não é formatado** automaticamente (ele pode guardar certificados e páginas). Use `cfg.formatarSeFalhar = true` só se o LittleFS for exclusivo do log.
- O *Upload Filesystem Image* substitui o LittleFS inteiro, **inclusive a pasta `/logs`**.

---

## English API

Every function has an English name that calls the same code. `debugInfo` and `debugVerbose` are the same in both languages.

```cpp
configureDebug(DEBUG_INFO, 4);
showDebugTime(true);
debugLittleFS.begin(DEBUG_WARNING);

debugError("Sensor timeout");
debugWarningf("Slow sensor (%d ms)", 250);
debugInfo("Ready");
debugTrace("Raw: " + String(raw));

int id = addDebugOutput([](int level, const char* msg) { /* ... */ }, DEBUG_ERROR);
```

| Português | English |
|:---|:---|
| `configurarDebug` | `configureDebug` |
| `definirNivelDebug` / `obterNivelDebugAtual` | `setDebugLevel` / `getDebugLevel` |
| `debugHabilitado` | `isDebugEnabled` |
| `definirPortaDebug` / `mostrarTempoDebug` | `setDebugPort` / `showDebugTime` |
| `adicionarSaidaDebug` / `definirNivelSaidaDebug` | `addDebugOutput` / `setDebugOutputLevel` |
| `removerSaidaDebug` / `removerSaidasDebug` | `removeDebugOutput` / `clearDebugOutputs` |
| `nomeNivelDebug` | `debugLevelName` |
| `debugErro` / `debugAviso` / `debugTudo` | `debugError` / `debugWarning` / `debugTrace` |
| `debugErrof` / `debugAvisof` / `debugTudof` | `debugErrorf` / `debugWarningf` / `debugTracef` |
| `SaidaDebug` | `DebugOutput` |
| `debugLittleFS.iniciar` / `atualizar` / `gravarAgora` | `begin` / `update` / `flush` |
| `definirNivel` / `sincronizarHorario` | `setLevel` / `syncTime` |
| `ultimasLinhas` / `paraCadaLinha` / `imprimirTudo` | `lastLines` / `forEachLine` / `printAll` |
| `limpar` / `tamanhoTotal` / `motivoReset` | `clear` / `totalSize` / `resetReason` |

---

## Referência

### Ajustes de compilação (`build_flags`)

| Define | Padrão | Descrição |
|:---|:---:|:---|
| `DEBUG_MANAGER_MAX_SAIDAS` | `4` | Saídas extras simultâneas |
| `DEBUG_MANAGER_TAMANHO_PRINTF` | `256` | Buffer das funções `...f()` |
| `DEBUG_MANAGER_USAR_STD_FUNCTION` | — | Força `std::function` em outras plataformas |

---

## Exemplos

| Exemplo | Descrição |
|:---|:---|
| `NiveisDeDebug` | Os 6 níveis e a mudança em tempo de execução |
| `02_SaidaExtra` | Duas saídas extras com níveis diferentes + `printf` |
| `03_LogLittleFS` | Log em arquivo com comandos pelo Serial (ESP32) |
| `04_EnglishAPI` | Tudo com os nomes em inglês (ESP32) |

---

## Histórico de versões

### 3.1.0
- **Saídas extras** com nível próprio (`adicionarSaidaDebug`, `definirNivelSaidaDebug`, `removerSaidaDebug`).
- **`DebugLittleFS`** (ESP32): log em arquivo com rotação, buffer em RAM, data/hora por NTP, motivo do reset e captura dos erros do ESP-IDF.
- Funções estilo **printf** (`debugInfof`...) e versões `const char*` que não criam String.
- `debugHabilitado()`, `definirPortaDebug()`, `mostrarTempoDebug()`, `nomeNivelDebug()`, `debugLog()`.
- `configurarDebug()` aceita a velocidade do Serial (ou `0` para não chamar `Serial.begin`) e espera o USB nativo (ESP32-S3) por até 1,5 s.
- `DEBUG_NENHUM` não imprime mais o cabeçalho; o cabeçalho avisa quando o GPIO forçou `DEBUG_TUDO`.
- Mutex no ESP32: tarefas diferentes não misturam linhas.
- **API em inglês** para todas as funções.
- Totalmente compatível com a 3.0.0.

### 3.0.0
- Biblioteca independente; nível e pino passados em `configurarDebug()`.

---

## Licença

MIT © 2026 [professorThiago](https://github.com/professorThiago)
