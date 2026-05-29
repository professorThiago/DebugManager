# DebugManager

Biblioteca Arduino/ESP32 para log em **6 níveis de verbosidade** via Serial.  
Leve, independente e reutilizável em qualquer projeto.

> Autor: [professorThiago](https://github.com/professorThiago)

---

## Níveis de verbosidade

Cada nível inclui todos os anteriores:

| Nível | Constante | Prefixo | Quando usar |
|-------|-----------|---------|-------------|
| 0 | `DEBUG_NENHUM` | — | Produção silenciosa |
| 1 | `DEBUG_ERRO` | `[ERRO]` | Falhas críticas que travam o funcionamento |
| 2 | `DEBUG_AVISO` | `[AVISO]` | Reconexões, timeouts, fila cheia, certificado ausente |
| 3 | `DEBUG_INFO` | `[INFO]` | WiFi conectou, MQTT publicou, setup concluído |
| 4 | `DEBUG_VERBOSE` | `[VERBOSE]` | Número de tentativas, transições de estado, configurações |
| 5 | `DEBUG_TUDO` | `[TUDO]` | Payloads brutos, códigos de erro numéricos, timings |

O nível inicial é definido em `secrets.h` via `DEBUG_NIVEL_INICIAL`.  
Para forçar `DEBUG_TUDO` sem alterar o código, puxe o pino `PINO_HABILITA_DEBUG_COMPLETO` para **GND** antes de ligar.

---

## Instalação

```ini
lib_deps =
    https://github.com/professorThiago/DebugManager
```

---

## Quick start

```cpp
#include <DebugManager.h>

void setup() {
    configurarDebug();   // Serial 115200 + lê pino de override

    debugErro  ("Falha ao inicializar sensor");
    debugAviso ("Reconectando ao WiFi...");
    debugInfo  ("WiFi conectado. IP: 192.168.1.100");
    debugVerbose("Tentativa 2/5, estado: CONECTANDO");
    debugTudo  ("Payload: {\"temp\":23.5,\"hum\":60}");
}
```

---

## API

### Inicialização

#### `void configurarDebug()`
Inicializa `Serial` (115200 baud), lê o pino de override e imprime o nível ativo.

### Log com nova linha

| Função | Nível mínimo |
|--------|-------------|
| `debugErro(msg)` | `DEBUG_ERRO` (1) |
| `debugAviso(msg)` | `DEBUG_AVISO` (2) |
| `debugInfo(msg)` | `DEBUG_INFO` (3) |
| `debugVerbose(msg)` | `DEBUG_VERBOSE` (4) |
| `debugTudo(msg)` | `DEBUG_TUDO` (5) |

### Controle de nível

```cpp
definirNivelDebug(DEBUG_NENHUM);   // silencia tudo
operacaoCritica();
definirNivelDebug(DEBUG_INFO);     // restaura

int atual = obterNivelDebugAtual();   // 0–5
```

---

## Sugestão de uso por ambiente

| Ambiente | Nível recomendado |
|----------|-------------------|
| Produção | `DEBUG_NENHUM` ou `DEBUG_ERRO` |
| Homologação | `DEBUG_AVISO` ou `DEBUG_INFO` |
| Desenvolvimento | `DEBUG_VERBOSE` |
| Debug de protocolo / rede | `DEBUG_TUDO` |

---

## Licença

MIT © 2026 [professorThiago](https://github.com/professorThiago)
