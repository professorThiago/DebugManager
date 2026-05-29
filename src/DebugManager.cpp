/**
 * @file DebugManager.cpp
 * @brief Implementação do sistema de log em 6 níveis.
 *
 * @author  professorThiago (https://github.com/professorThiago)
 * @version 3.0.0
 * @license MIT
 */

#include "DebugManager.h"

static int nivelDebugAtual = DEBUG_INFO;

// =============================================================================
// Inicialização
// =============================================================================

void configurarDebug(int nivelInicial, int pinoOverride)
{
    Serial.begin(115200);
    delay(200);

    if (pinoOverride >= 0)
    {
        pinMode(pinoOverride, INPUT_PULLUP);
        nivelDebugAtual = (digitalRead(pinoOverride) == LOW) ? DEBUG_TUDO : nivelInicial;
    }
    else
    {
        nivelDebugAtual = nivelInicial;
    }

    Serial.print("\n\n\n");
    Serial.println("[INFO] ================================");
    Serial.println("[INFO]  ESP32 — DebugManager v3.0.0");
    Serial.print  ("[INFO]  Nivel ativo: ");

    switch (nivelDebugAtual)
    {
        case DEBUG_NENHUM:  Serial.println("0 - NENHUM");  break;
        case DEBUG_ERRO:    Serial.println("1 - ERRO");    break;
        case DEBUG_AVISO:   Serial.println("2 - AVISO");   break;
        case DEBUG_INFO:    Serial.println("3 - INFO");    break;
        case DEBUG_VERBOSE: Serial.println("4 - VERBOSE"); break;
        case DEBUG_TUDO:    Serial.println("5 - TUDO");    break;
        default:            Serial.println("?");           break;
    }

    Serial.println("[INFO] ================================");
}

// =============================================================================
// Controle de nível
// =============================================================================

void definirNivelDebug(int nivel) { nivelDebugAtual = nivel; }
int  obterNivelDebugAtual()       { return nivelDebugAtual;  }

// =============================================================================
// Funções de log
// =============================================================================

void debugErro(const String& msg)
    { if (nivelDebugAtual >= DEBUG_ERRO)    { Serial.print("[ERRO]    "); Serial.println(msg); } }

void debugAviso(const String& msg)
    { if (nivelDebugAtual >= DEBUG_AVISO)   { Serial.print("[AVISO]   "); Serial.println(msg); } }

void debugInfo(const String& msg)
    { if (nivelDebugAtual >= DEBUG_INFO)    { Serial.print("[INFO]    "); Serial.println(msg); } }

void debugVerbose(const String& msg)
    { if (nivelDebugAtual >= DEBUG_VERBOSE) { Serial.print("[VERBOSE] "); Serial.println(msg); } }

void debugTudo(const String& msg)
    { if (nivelDebugAtual >= DEBUG_TUDO)    { Serial.print("[TUDO]    "); Serial.println(msg); } }
