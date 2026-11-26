/**
 * @file DebugManager.cpp
 * @brief Implementação do sistema de log em 6 níveis com saídas extras.
 *
 * @author  professorThiago (https://github.com/professorThiago)
 * @version 3.1.0
 * @license MIT
 */

#include "DebugManager.h"
#include <stdarg.h>

#if defined(ARDUINO_ARCH_ESP32)
  #include <freertos/FreeRTOS.h>
  #include <freertos/semphr.h>
  // Mutex recursivo: várias tarefas podem logar sem misturar as linhas,
  // e uma saída extra pode chamar debug*() sem travar a própria tarefa.
  static SemaphoreHandle_t mutexDebug = nullptr;
  #define TRAVAR()   do { if (mutexDebug) xSemaphoreTakeRecursive(mutexDebug, portMAX_DELAY); } while (0)
  #define LIBERAR()  do { if (mutexDebug) xSemaphoreGiveRecursive(mutexDebug); } while (0)
#else
  #define TRAVAR()   do {} while (0)
  #define LIBERAR()  do {} while (0)
#endif

static int    nivelDebugAtual = DEBUG_INFO;
static Print* portaDebug      = &Serial;
static bool   mostrarTempo    = false;

struct SaidaRegistrada {
    SaidaDebug saida;
    int        nivel;
    bool       ativa;
};
static SaidaRegistrada saidas[DEBUG_MANAGER_MAX_SAIDAS];
static int  nivelMaxSaidas = -1;     // maior nível pedido por alguma saída extra
static bool dentroDeSaida  = false;  // evita loop quando a saída extra chama debug*()

static const char* const PREFIXOS[] = {
    "", "[ERRO]    ", "[AVISO]   ", "[INFO]    ", "[VERBOSE] ", "[TUDO]    "
};

// =============================================================================
// Configuração
// =============================================================================

void configurarDebug(int nivelInicial, int pinoOverride, unsigned long velocidade)
{
#if defined(ARDUINO_ARCH_ESP32)
    if (!mutexDebug) mutexDebug = xSemaphoreCreateRecursiveMutex();
#endif

    if (velocidade > 0)
    {
        Serial.begin(velocidade);
        // Placas com USB nativo (ESP32-S3, Leonardo): espera o monitor até 1,5 s
        const uint32_t inicio = millis();
        while (!Serial && millis() - inicio < 1500) delay(10);
        delay(100);
    }

    if (pinoOverride >= 0)
    {
        pinMode(pinoOverride, INPUT_PULLUP);
        delay(2);   // estabiliza o pull-up antes de ler
        nivelDebugAtual = (digitalRead(pinoOverride) == LOW) ? DEBUG_TUDO : nivelInicial;
    }
    else
    {
        nivelDebugAtual = nivelInicial;
    }

    if (nivelDebugAtual == DEBUG_NENHUM) return;   // silencioso de verdade

    portaDebug->print("\n\n");
    portaDebug->println("[INFO]    ================================");
    portaDebug->println("[INFO]     DebugManager v" DEBUG_MANAGER_VERSAO);
    portaDebug->print  ("[INFO]     Nivel ativo: ");
    portaDebug->print  (nivelDebugAtual);
    portaDebug->print  (" - ");
    portaDebug->println(nomeNivelDebug(nivelDebugAtual));
    if (pinoOverride >= 0 && nivelDebugAtual == DEBUG_TUDO && nivelInicial != DEBUG_TUDO)
    {
        portaDebug->print  ("[INFO]     (forcado pelo GPIO ");
        portaDebug->print  (pinoOverride);
        portaDebug->println(")");
    }
    portaDebug->println("[INFO]    ================================");
}

void definirNivelDebug(int nivel)
{
    if (nivel < DEBUG_NENHUM) nivel = DEBUG_NENHUM;
    if (nivel > DEBUG_TUDO)   nivel = DEBUG_TUDO;
    nivelDebugAtual = nivel;
}

int  obterNivelDebugAtual()      { return nivelDebugAtual; }
void definirPortaDebug(Print& p) { portaDebug = &p; }
void mostrarTempoDebug(bool a)   { mostrarTempo = a; }

bool debugHabilitado(int nivel)
{
    if (nivel <= DEBUG_NENHUM) return false;
    return nivel <= nivelDebugAtual || nivel <= nivelMaxSaidas;
}

const char* nomeNivelDebug(int nivel)
{
    switch (nivel)
    {
        case DEBUG_NENHUM:  return "NENHUM";
        case DEBUG_ERRO:    return "ERRO";
        case DEBUG_AVISO:   return "AVISO";
        case DEBUG_INFO:    return "INFO";
        case DEBUG_VERBOSE: return "VERBOSE";
        case DEBUG_TUDO:    return "TUDO";
        default:            return "?";
    }
}

// =============================================================================
// Saídas extras
// =============================================================================

static void recalcularNivelSaidas()
{
    nivelMaxSaidas = -1;
    for (int i = 0; i < DEBUG_MANAGER_MAX_SAIDAS; i++)
        if (saidas[i].ativa && saidas[i].nivel > nivelMaxSaidas) nivelMaxSaidas = saidas[i].nivel;
}

int adicionarSaidaDebug(SaidaDebug saida, int nivelMinimo)
{
    if (!saida) return -1;
    TRAVAR();
    int id = -1;
    for (int i = 0; i < DEBUG_MANAGER_MAX_SAIDAS; i++)
    {
        if (!saidas[i].ativa)
        {
            saidas[i].saida = saida;
            saidas[i].nivel = nivelMinimo;
            saidas[i].ativa = true;
            id = i;
            break;
        }
    }
    recalcularNivelSaidas();
    LIBERAR();
    return id;
}

void definirNivelSaidaDebug(int id, int nivelMinimo)
{
    if (id < 0 || id >= DEBUG_MANAGER_MAX_SAIDAS) return;
    TRAVAR();
    saidas[id].nivel = nivelMinimo;
    recalcularNivelSaidas();
    LIBERAR();
}

void removerSaidaDebug(int id)
{
    if (id < 0 || id >= DEBUG_MANAGER_MAX_SAIDAS) return;
    TRAVAR();
    saidas[id].ativa = false;
    saidas[id].saida = nullptr;
    recalcularNivelSaidas();
    LIBERAR();
}

void removerSaidasDebug()
{
    TRAVAR();
    for (int i = 0; i < DEBUG_MANAGER_MAX_SAIDAS; i++)
    {
        saidas[i].ativa = false;
        saidas[i].saida = nullptr;
    }
    nivelMaxSaidas = -1;
    LIBERAR();
}

// =============================================================================
// Núcleo
// =============================================================================

void debugLog(int nivel, const char* msg)
{
    if (nivel <= DEBUG_NENHUM || nivel > DEBUG_TUDO || !msg) return;
    if (!debugHabilitado(nivel)) return;

    TRAVAR();

    if (nivel <= nivelDebugAtual)
    {
        if (mostrarTempo)
        {
            char tempo[16];
            const uint32_t ms = millis();
            snprintf(tempo, sizeof(tempo), "[%5lu.%03lu] ",
                     (unsigned long)(ms / 1000), (unsigned long)(ms % 1000));
            portaDebug->print(tempo);
        }
        portaDebug->print(PREFIXOS[nivel]);
        portaDebug->println(msg);
    }

    if (!dentroDeSaida && nivel <= nivelMaxSaidas)
    {
        dentroDeSaida = true;
        for (int i = 0; i < DEBUG_MANAGER_MAX_SAIDAS; i++)
            if (saidas[i].ativa && nivel <= saidas[i].nivel && saidas[i].saida)
                saidas[i].saida(nivel, msg);
        dentroDeSaida = false;
    }

    LIBERAR();
}

static void debugFormatado(int nivel, const char* formato, va_list args)
{
    if (!debugHabilitado(nivel) || !formato) return;
    char buf[DEBUG_MANAGER_TAMANHO_PRINTF];
    vsnprintf(buf, sizeof(buf), formato, args);
    debugLog(nivel, buf);
}

// =============================================================================
// Funções de log
// =============================================================================

void debugErro   (const char* m)   { debugLog(DEBUG_ERRO,    m); }
void debugAviso  (const char* m)   { debugLog(DEBUG_AVISO,   m); }
void debugInfo   (const char* m)   { debugLog(DEBUG_INFO,    m); }
void debugVerbose(const char* m)   { debugLog(DEBUG_VERBOSE, m); }
void debugTudo   (const char* m)   { debugLog(DEBUG_TUDO,    m); }

void debugErro   (const String& m) { debugLog(DEBUG_ERRO,    m.c_str()); }
void debugAviso  (const String& m) { debugLog(DEBUG_AVISO,   m.c_str()); }
void debugInfo   (const String& m) { debugLog(DEBUG_INFO,    m.c_str()); }
void debugVerbose(const String& m) { debugLog(DEBUG_VERBOSE, m.c_str()); }
void debugTudo   (const String& m) { debugLog(DEBUG_TUDO,    m.c_str()); }

#define DEFINIR_PRINTF(nome, nivel)                     \
    void nome(const char* formato, ...)                 \
    {                                                   \
        va_list args;                                   \
        va_start(args, formato);                        \
        debugFormatado(nivel, formato, args);           \
        va_end(args);                                   \
    }

DEFINIR_PRINTF(debugErrof,    DEBUG_ERRO)
DEFINIR_PRINTF(debugAvisof,   DEBUG_AVISO)
DEFINIR_PRINTF(debugInfof,    DEBUG_INFO)
DEFINIR_PRINTF(debugVerbosef, DEBUG_VERBOSE)
DEFINIR_PRINTF(debugTudof,    DEBUG_TUDO)

DEFINIR_PRINTF(debugErrorf,   DEBUG_ERRO)
DEFINIR_PRINTF(debugWarningf, DEBUG_AVISO)
DEFINIR_PRINTF(debugTracef,   DEBUG_TUDO)
