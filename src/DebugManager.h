/**
 * @file DebugManager.h
 * @brief Log em 6 níveis para Arduino/ESP32, com saídas extras (arquivo,
 *        MQTT, display...) e API em português e inglês.
 *
 * | Nível | PT              | EN              | Prefixo     | Quando usar                          |
 * |-------|-----------------|-----------------|-------------|--------------------------------------|
 * | 0     | `DEBUG_NENHUM`  | `DEBUG_NONE`    | —           | Produção silenciosa                  |
 * | 1     | `DEBUG_ERRO`    | `DEBUG_ERROR`   | `[ERRO]`    | Falhas que afetam o funcionamento    |
 * | 2     | `DEBUG_AVISO`   | `DEBUG_WARNING` | `[AVISO]`   | Reconexões, timeouts, fila cheia     |
 * | 3     | `DEBUG_INFO`    | `DEBUG_INFO`    | `[INFO]`    | Fluxo normal (conectou, publicou)    |
 * | 4     | `DEBUG_VERBOSE` | `DEBUG_VERBOSE` | `[VERBOSE]` | Tentativas, transições de estado     |
 * | 5     | `DEBUG_TUDO`    | `DEBUG_TRACE`   | `[TUDO]`    | Payloads brutos, códigos numéricos   |
 *
 * @par Uso básico
 * @code
 * #include <DebugManager.h>
 *
 * void setup() {
 *     configurarDebug(DEBUG_INFO, 4);          // nível INFO; GPIO 4 no GND ao ligar → TUDO
 *     debugInfo("Sistema pronto");
 *     debugInfof("Memoria livre: %u bytes", ESP.getFreeHeap());
 * }
 * @endcode
 *
 * @par Saída extra (ex.: gravar em arquivo, enviar por MQTT)
 * @code
 * adicionarSaidaDebug([](int nivel, const char* msg) {
 *     // recebe cada mensagem com nível <= DEBUG_AVISO
 * }, DEBUG_AVISO);
 * @endcode
 *
 * Para gravar no LittleFS (ESP32), veja DebugLittleFS.h.
 *
 * @author  professorThiago (https://github.com/professorThiago)
 * @version 3.1.0
 * @license MIT
 */

#ifndef DEBUG_MANAGER_H
#define DEBUG_MANAGER_H

#include <Arduino.h>

#define DEBUG_MANAGER_VERSAO  "3.1.0"
#define DEBUG_MANAGER_VERSION DEBUG_MANAGER_VERSAO

// ---------------------------------------------------------------------------
// Níveis (PT e EN têm o mesmo valor)
// ---------------------------------------------------------------------------

#define DEBUG_NENHUM   0
#define DEBUG_ERRO     1
#define DEBUG_AVISO    2
#define DEBUG_INFO     3
#define DEBUG_VERBOSE  4
#define DEBUG_TUDO     5

#define DEBUG_NONE     DEBUG_NENHUM
#define DEBUG_ERROR    DEBUG_ERRO
#define DEBUG_WARNING  DEBUG_AVISO
#define DEBUG_TRACE    DEBUG_TUDO
#define DEBUG_ALL      DEBUG_TUDO

// ---------------------------------------------------------------------------
// Ajustes de compilação (build_flags)
// ---------------------------------------------------------------------------

/** @brief Máximo de saídas extras registradas ao mesmo tempo. */
#ifndef DEBUG_MANAGER_MAX_SAIDAS
  #define DEBUG_MANAGER_MAX_SAIDAS 4
#endif

/** @brief Tamanho do buffer das funções estilo printf (debugInfof...). */
#ifndef DEBUG_MANAGER_TAMANHO_PRINTF
  #define DEBUG_MANAGER_TAMANHO_PRINTF 256
#endif

// ---------------------------------------------------------------------------
// Saída extra
//   ESP32 / ESP8266 / RP2040 → std::function (aceita lambdas com captura)
//   AVR e demais             → ponteiro de função
// ---------------------------------------------------------------------------

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266) || \
    defined(ARDUINO_ARCH_RP2040) || defined(DEBUG_MANAGER_USAR_STD_FUNCTION)
  #include <functional>
  typedef std::function<void(int nivel, const char* mensagem)> SaidaDebug;
#else
  typedef void (*SaidaDebug)(int nivel, const char* mensagem);
#endif
typedef SaidaDebug DebugOutput;   ///< EN

// ---------------------------------------------------------------------------
// Configuração
// ---------------------------------------------------------------------------

/**
 * @brief Inicializa o debug (chame no início do setup()).
 *
 * @param nivelInicial  Nível do Serial (DEBUG_NENHUM … DEBUG_TUDO).
 * @param pinoOverride  GPIO com pull-up interno. Se estiver no GND ao ligar,
 *                      força DEBUG_TUDO. Use -1 para desativar.
 * @param velocidade    Baud rate do Serial (padrão 115200). Use 0 se o seu
 *                      código já chamou Serial.begin().
 */
void configurarDebug(int nivelInicial = DEBUG_INFO, int pinoOverride = -1,
                     unsigned long velocidade = 115200);

/** @brief Muda o nível do Serial em tempo de execução. */
void definirNivelDebug(int nivel);

/** @brief Nível atual do Serial (0–5). */
int  obterNivelDebugAtual();

/**
 * @brief `true` se uma mensagem desse nível vai para algum lugar (Serial ou saída extra).
 *
 * Use para evitar montar textos caros que seriam descartados:
 * @code
 * if (debugHabilitado(DEBUG_TUDO)) debugTudo("Payload: " + payloadGigante);
 * @endcode
 */
bool debugHabilitado(int nivel);

/** @brief Troca a porta de saída (padrão: Serial). Ex.: definirPortaDebug(Serial1). */
void definirPortaDebug(Print& porta);

/** @brief Prefixa cada linha do Serial com o tempo desde o boot: `[  12.345]`. */
void mostrarTempoDebug(bool ativo);

/**
 * @brief Registra uma saída extra: recebe cada mensagem com nível <= `nivelMinimo`.
 *
 * O nível da saída extra é independente do nível do Serial: dá para gravar
 * avisos em arquivo com o Serial silencioso, ou ver tudo no Serial e gravar
 * só erros.
 *
 * @return Identificador da saída (0, 1, 2...) ou -1 se já houver
 *         DEBUG_MANAGER_MAX_SAIDAS saídas registradas.
 *
 * @note Uma mensagem gerada DENTRO de uma saída extra (ex.: debugErro() chamado
 *       pelo próprio callback) vai só para o Serial, para não entrar em loop.
 */
int  adicionarSaidaDebug(SaidaDebug saida, int nivelMinimo = DEBUG_INFO);

/** @brief Muda o nível de uma saída extra (id devolvido por adicionarSaidaDebug). */
void definirNivelSaidaDebug(int id, int nivelMinimo);

/** @brief Remove uma saída extra. */
void removerSaidaDebug(int id);

/** @brief Remove todas as saídas extras. */
void removerSaidasDebug();

/** @brief Nome do nível: "ERRO", "AVISO", "INFO", "VERBOSE", "TUDO". */
const char* nomeNivelDebug(int nivel);

// ---------------------------------------------------------------------------
// Funções de log
// ---------------------------------------------------------------------------

void debugErro   (const String& msg);   ///< `[ERRO]`    nível >= 1
void debugAviso  (const String& msg);   ///< `[AVISO]`   nível >= 2
void debugInfo   (const String& msg);   ///< `[INFO]`    nível >= 3
void debugVerbose(const String& msg);   ///< `[VERBOSE]` nível >= 4
void debugTudo   (const String& msg);   ///< `[TUDO]`    nível >= 5

// Versões para texto fixo — não criam String (economizam memória)
void debugErro   (const char* msg);
void debugAviso  (const char* msg);
void debugInfo   (const char* msg);
void debugVerbose(const char* msg);
void debugTudo   (const char* msg);

// Estilo printf: debugInfof("Temperatura: %.1f C", t);
void debugErrof   (const char* formato, ...);
void debugAvisof  (const char* formato, ...);
void debugInfof   (const char* formato, ...);
void debugVerbosef(const char* formato, ...);
void debugTudof   (const char* formato, ...);

/** @brief Envia com nível escolhido em tempo de execução. */
void debugLog(int nivel, const char* msg);

// ---------------------------------------------------------------------------
// English API — same functions, English names
// ---------------------------------------------------------------------------

inline void configureDebug(int level = DEBUG_INFO, int overridePin = -1,
                           unsigned long baud = 115200)        { configurarDebug(level, overridePin, baud); }
inline void setDebugLevel(int level)                           { definirNivelDebug(level); }
inline int  getDebugLevel()                                    { return obterNivelDebugAtual(); }
inline bool isDebugEnabled(int level)                          { return debugHabilitado(level); }
inline void setDebugPort(Print& port)                          { definirPortaDebug(port); }
inline void showDebugTime(bool on)                             { mostrarTempoDebug(on); }
inline int  addDebugOutput(DebugOutput out, int minLevel = DEBUG_INFO) { return adicionarSaidaDebug(out, minLevel); }
inline void setDebugOutputLevel(int id, int minLevel)          { definirNivelSaidaDebug(id, minLevel); }
inline void removeDebugOutput(int id)                          { removerSaidaDebug(id); }
inline void clearDebugOutputs()                                { removerSaidasDebug(); }
inline const char* debugLevelName(int level)                   { return nomeNivelDebug(level); }

inline void debugError  (const String& m) { debugErro(m); }
inline void debugWarning(const String& m) { debugAviso(m); }
inline void debugTrace  (const String& m) { debugTudo(m); }
inline void debugError  (const char* m)   { debugErro(m); }
inline void debugWarning(const char* m)   { debugAviso(m); }
inline void debugTrace  (const char* m)   { debugTudo(m); }
// debugInfo() and debugVerbose() are the same word in both languages.

void debugErrorf  (const char* format, ...);
void debugWarningf(const char* format, ...);
void debugTracef  (const char* format, ...);
// debugInfof() and debugVerbosef() are the same in both languages.

#endif // DEBUG_MANAGER_H
