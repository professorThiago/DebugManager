/**
 * @file 03_LogLittleFS.ino
 * @brief DebugManager — Exemplo 3: gravar o log em arquivo no LittleFS (ESP32).
 *
 * Tudo o que passa por debugErro(), debugAviso(), debugInfo()... vai também
 * para /logs/atual.log, com data e hora — inclusive as mensagens das
 * bibliotecas que usam o DebugManager (ex.: ConectividadeESP32).
 *
 * platformio.ini:
 *   board_build.filesystem = littlefs
 *
 * Primeira vez: se o LittleFS nunca foi usado nessa placa, faça um
 * "Upload Filesystem Image" (pode ser com a pasta data/ vazia) ou use
 * cfg.formatarSeFalhar = true.
 *
 * Comandos no Monitor Serial:
 *   log         → mostra todos os arquivos
 *   ultimas 20  → últimas 20 linhas
 *   limpar      → apaga os logs
 *   nivel 3     → grava até INFO no arquivo (1 = erro ... 5 = tudo)
 *
 * @author  professorThiago (https://github.com/professorThiago)
 * @license MIT
 */

#include <Arduino.h>
#include <DebugManager.h>
#include <DebugLittleFS.h>

void setup() {
    configurarDebug(DEBUG_INFO);              // Serial mostra até INFO

    DebugLittleFS::Config cfg;
    cfg.nivel             = DEBUG_AVISO;      // arquivo grava só avisos e erros
    cfg.tamanhoMaxArquivo = 32 * 1024;        // gira a cada 32 KB
    cfg.arquivosAntigos   = 3;                // atual + 3 antigos
    debugLittleFS.iniciar(cfg);

    // A primeira linha do arquivo já traz o motivo do último reset
    debugInfof("Motivo do reset: %s", DebugLittleFS::motivoReset());
    debugInfof("Logs ocupam %u bytes", (unsigned)debugLittleFS.tamanhoTotal());
}

void lerComando() {
    if (!Serial.available()) return;
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "log")                 debugLittleFS.imprimirTudo(Serial);
    else if (cmd.startsWith("ultimas")) Serial.print(debugLittleFS.ultimasLinhas(cmd.substring(8).toInt() > 0 ? cmd.substring(8).toInt() : 20));
    else if (cmd == "limpar")         { debugLittleFS.limpar(); debugAviso("Logs apagados"); }
    else if (cmd.startsWith("nivel")) { debugLittleFS.definirNivel(cmd.substring(6).toInt()); debugInfo("Nivel do arquivo alterado"); }
}

void loop() {
    debugLittleFS.atualizar();    // grava o buffer na flash periodicamente
    lerComando();

    static uint32_t ultimo = 0;
    static int n = 0;
    if (millis() - ultimo >= 5000) {
        ultimo = millis();
        n++;
        debugInfof("Leitura %d", n);                       // Serial: sim | arquivo: não (nível AVISO)
        if (n % 4 == 0) debugAvisof("Leitura %d fora da faixa", n);   // Serial e arquivo
    }
}
