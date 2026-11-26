/**
 * @file NiveisDeDebug.ino
 * @brief DebugManager — Exemplo: demonstra os 6 níveis de verbosidade.
 *
 * @details
 * Alterna o nível a cada ciclo para mostrar quais mensagens aparecem
 * em cada configuração. Observe que cada nível inclui todos os anteriores.
 *
 * Saída esperada com nível 3 (DEBUG_INFO):
 *   [ERRO]    mensagem de erro
 *   [AVISO]   mensagem de aviso
 *   [INFO]    mensagem de info
 *   (verbose e tudo não aparecem)
 *
 * @author  professorThiago (https://github.com/professorThiago)
 * @license MIT
 */

#include <Arduino.h>
#include <DebugManager.h>

void enviarTodasMensagens() {
    debugErro  ("mensagem de ERRO   — algo critico aconteceu");
    debugAviso ("mensagem de AVISO  — situacao anormal mas recuperavel");
    debugInfo  ("mensagem de INFO   — fluxo normal do sistema");
    debugVerbose("mensagem VERBOSE  — detalhes internos e tentativas");
    debugTudo  ("mensagem TUDO     — dados brutos, payloads, codigos");
}

void setup() {
    configurarDebug();
}

void loop() {
    for (int nivel = DEBUG_TUDO; nivel >= DEBUG_NENHUM; nivel--) {
        definirNivelDebug(nivel);

        Serial.println("\n========================================");
        Serial.print  ("Nivel ativo: ");
        Serial.println(nivel);
        Serial.println("========================================");

        enviarTodasMensagens();
        delay(3000);
    }

    // Demonstra mudança em tempo de execução
    definirNivelDebug(DEBUG_INFO);
    debugInfo("Nivel restaurado para DEBUG_INFO.");
    debugVerbose("Esta linha NAO aparece no nivel INFO.");
    delay(2000);
}
