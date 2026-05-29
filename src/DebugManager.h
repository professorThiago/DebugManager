/**
 * @file DebugManager.h
 * @brief Sistema de log em 6 níveis para ESP32.
 *
 * @details
 * Fornece funções de log com seis níveis de verbosidade crescente.
 * Cada nível inclui todos os anteriores:
 *
 * | Nível | Constante        | Exibe                                          |
 * |-------|------------------|------------------------------------------------|
 * | 0     | `DEBUG_NENHUM`   | Nada — produção silenciosa                     |
 * | 1     | `DEBUG_ERRO`     | Erros críticos que afetam o funcionamento      |
 * | 2     | `DEBUG_AVISO`    | + Avisos (reconexões, fila cheia, timeouts)    |
 * | 3     | `DEBUG_INFO`     | + Fluxo normal (conectou, publicou, inscreveu) |
 * | 4     | `DEBUG_VERBOSE`  | + Detalhes internos (tentativas, estados)      |
 * | 5     | `DEBUG_TUDO`     | + Dados brutos (payloads, códigos de erro)     |
 *
 * O nível inicial é definido em `secrets.h` via `DEBUG_NIVEL_INICIAL`.
 * Pode ser forçado para `DEBUG_TUDO` puxando o pino
 * `PINO_HABILITA_DEBUG_COMPLETO` para GND antes de ligar.
 *
 * @par Uso básico
 * @code
 * #include <DebugManager.h>
 *
 * void setup() {
 *     configurarDebug();
 *     debugErro("Falha crítica");
 *     debugAviso("Reconectando...");
 *     debugInfo("Conectado ao WiFi");
 *     debugVerbose("Tentativa 2/5, estado: CONECTANDO");
 *     debugTudo("Payload bruto: {\"v\":42}");
 * }
 * @endcode
 *
 * @author  professorThiago (https://github.com/professorThiago)
 * @version 2.0.0
 * @date    2026
 * @license MIT
 */

#ifndef DEBUG_MANAGER_H
#define DEBUG_MANAGER_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Níveis de verbosidade
// ---------------------------------------------------------------------------

#define DEBUG_NENHUM   0  ///< Sem saída serial alguma (produção)
#define DEBUG_ERRO     1  ///< Apenas erros críticos
#define DEBUG_AVISO    2  ///< Erros + avisos (reconexões, timeouts, fila cheia)
#define DEBUG_INFO     3  ///< Aviso + fluxo normal (conectou, publicou)
#define DEBUG_VERBOSE  4  ///< Info + detalhes internos (tentativas, estados)
#define DEBUG_TUDO     5  ///< Verbose + dados brutos (payloads, códigos de erro)

// ---------------------------------------------------------------------------
// API pública — inicialização
// ---------------------------------------------------------------------------

/**
 * @brief Inicializa o sistema de debug.
 *
 * Deve ser chamado uma vez no `setup()`. Realiza:
 *  - `Serial.begin(115200)`
 *  - Lê o pino `PINO_HABILITA_DEBUG_COMPLETO` (INPUT_PULLUP).
 *    Se estiver em LOW, força `DEBUG_TUDO` independente de `secrets.h`.
 *  - Imprime um cabeçalho indicando o nível ativo.
 */
void configurarDebug();

// ---------------------------------------------------------------------------
// API pública — log com nova linha
// ---------------------------------------------------------------------------

/**
 * @brief Imprime `[ERRO] msg` + nova linha. Ativo quando nível ≥ `DEBUG_ERRO`.
 * @param msg  Texto a exibir.
 */
void debugErro(const String& msg);

/**
 * @brief Imprime `[AVISO] msg` + nova linha. Ativo quando nível ≥ `DEBUG_AVISO`.
 *
 * Use para situações anormais mas recuperáveis: reconexão WiFi, fila cheia,
 * timeout de conexão, certificado ausente.
 *
 * @param msg  Texto a exibir.
 */
void debugAviso(const String& msg);

/**
 * @brief Imprime `[INFO] msg` + nova linha. Ativo quando nível ≥ `DEBUG_INFO`.
 *
 * Use para eventos esperados do fluxo normal: WiFi conectou, MQTT inscreveu,
 * mensagem publicada, setup concluído.
 *
 * @param msg  Texto a exibir.
 */
void debugInfo(const String& msg);

/**
 * @brief Imprime `[VERBOSE] msg` + nova linha. Ativo quando nível ≥ `DEBUG_VERBOSE`.
 *
 * Use para detalhes internos úteis durante o desenvolvimento: número da
 * tentativa, transições de estado da máquina, configuração de certificados.
 *
 * @param msg  Texto a exibir.
 */
void debugVerbose(const String& msg);

/**
 * @brief Imprime `[TUDO] msg` + nova linha. Ativo quando nível ≥ `DEBUG_TUDO`.
 *
 * Use para dados brutos: payloads MQTT completos, códigos de erro numéricos,
 * timings, buffers de rede.
 *
 * @param msg  Texto a exibir.
 */
void debugTudo(const String& msg);

// ---------------------------------------------------------------------------
// API pública — log sem nova linha (para construção incremental de linha)
// ---------------------------------------------------------------------------

/** @brief `[ERRO]` sem nova linha. @param msg Texto a exibir. */
void debugErroSemLinha(const String& msg);

/** @brief `[AVISO]` sem nova linha. @param msg Texto a exibir. */
void debugAvisoSemLinha(const String& msg);

/** @brief `[INFO]` sem nova linha. @param msg Texto a exibir. */
void debugInfoSemLinha(const String& msg);

/** @brief `[VERBOSE]` sem nova linha. @param msg Texto a exibir. */
void debugVerboseSemLinha(const String& msg);

/** @brief `[TUDO]` sem nova linha. @param msg Texto a exibir. */
void debugTudoSemLinha(const String& msg);

// ---------------------------------------------------------------------------
// API pública — controle de nível
// ---------------------------------------------------------------------------

/**
 * @brief Retorna o nível de debug atualmente ativo (0–5).
 */
int obterNivelDebugAtual();

/**
 * @brief Altera o nível de debug em tempo de execução.
 *
 * @param nivel  Um dos valores `DEBUG_NENHUM` … `DEBUG_TUDO`.
 *
 * @par Exemplo — silencia durante operação crítica
 * @code
 * definirNivelDebug(DEBUG_NENHUM);
 * operacaoCritica();
 * definirNivelDebug(DEBUG_INFO);
 * @endcode
 */
void definirNivelDebug(int nivel);

#endif // DEBUG_MANAGER_H
