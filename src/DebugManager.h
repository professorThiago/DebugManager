/**
 * @file DebugManager.h
 * @brief Sistema de log em 6 níveis para ESP32.
 *
 * @details
 * Biblioteca independente — não depende de nenhum arquivo do projeto.
 * O nível e o pino de override são passados diretamente em `configurarDebug()`.
 *
 * | Nível | Constante        | Prefixo    | Quando usar                                    |
 * |-------|------------------|------------|------------------------------------------------|
 * | 0     | `DEBUG_NENHUM`   | —          | Produção silenciosa                            |
 * | 1     | `DEBUG_ERRO`     | `[ERRO]`   | Falhas críticas que afetam o funcionamento     |
 * | 2     | `DEBUG_AVISO`    | `[AVISO]`  | Reconexões, timeouts, fila cheia               |
 * | 3     | `DEBUG_INFO`     | `[INFO]`   | Fluxo normal (conectou, publicou)              |
 * | 4     | `DEBUG_VERBOSE`  | `[VERBOSE]`| Tentativas, transições de estado               |
 * | 5     | `DEBUG_TUDO`     | `[TUDO]`   | Payloads brutos, códigos de erro numéricos     |
 *
 * @par Uso básico
 * @code
 * #include <DebugManager.h>
 *
 * void setup() {
 *     configurarDebug(3, 4);   // nível INFO, override no GPIO 4
 *     debugInfo("Sistema pronto");
 * }
 * @endcode
 *
 * @author  professorThiago (https://github.com/professorThiago)
 * @version 3.0.0
 * @license MIT
 */

#ifndef DEBUG_MANAGER_H
#define DEBUG_MANAGER_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Níveis de verbosidade
// ---------------------------------------------------------------------------

#define DEBUG_NENHUM   0
#define DEBUG_ERRO     1
#define DEBUG_AVISO    2
#define DEBUG_INFO     3
#define DEBUG_VERBOSE  4
#define DEBUG_TUDO     5

// ---------------------------------------------------------------------------
// API pública
// ---------------------------------------------------------------------------

/**
 * @brief Inicializa o sistema de debug.
 *
 * @param nivelInicial  Nível padrão ao ligar (0–5). Use as constantes DEBUG_*.
 * @param pinoOverride  GPIO com pull-up interno. Se puxado para GND antes de
 *                      ligar, força `DEBUG_TUDO` independente de `nivelInicial`.
 *                      Passe -1 para desabilitar este recurso.
 *
 * @par Exemplo
 * @code
 * configurarDebug(DEBUG_INFO, 4);    // INFO por padrão, override no GPIO 4
 * configurarDebug(DEBUG_NENHUM, -1); // silencioso, sem override
 * @endcode
 */
void configurarDebug(int nivelInicial = DEBUG_INFO, int pinoOverride = -1);

/**
 * @brief Altera o nível de debug em tempo de execução.
 * @param nivel  DEBUG_NENHUM … DEBUG_TUDO.
 */
void definirNivelDebug(int nivel);

/** @brief Retorna o nível de debug atualmente ativo (0–5). */
int obterNivelDebugAtual();

// ---------------------------------------------------------------------------
// Funções de log
// ---------------------------------------------------------------------------

void debugErro   (const String& msg); ///< `[ERRO]`    nível ≥ 1
void debugAviso  (const String& msg); ///< `[AVISO]`   nível ≥ 2
void debugInfo   (const String& msg); ///< `[INFO]`    nível ≥ 3
void debugVerbose(const String& msg); ///< `[VERBOSE]` nível ≥ 4
void debugTudo   (const String& msg); ///< `[TUDO]`    nível ≥ 5

#endif // DEBUG_MANAGER_H
