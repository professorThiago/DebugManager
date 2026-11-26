/**
 * @file 02_SaidaExtra.ino
 * @brief DebugManager — Exemplo 2: saídas extras.
 *
 * Uma saída extra recebe as mensagens do DebugManager para fazer o que você
 * quiser: gravar em arquivo, publicar por MQTT, mostrar num display, piscar
 * um LED... Cada saída tem o SEU nível, independente do Serial.
 *
 * Neste exemplo:
 *  - o Serial mostra tudo até INFO;
 *  - a saída 1 conta erros e acende o LED quando aparece um;
 *  - a saída 2 guarda os 5 últimos avisos/erros na RAM (um "histórico").
 *
 * @author  professorThiago (https://github.com/professorThiago)
 * @license MIT
 */

#include <Arduino.h>
#include <DebugManager.h>

const int PINO_LED = 2;

int    totalErros = 0;
String historico[5];
int    posHistorico = 0;

// Saída 1: reage a erros
void saidaErros(int nivel, const char* mensagem) {
    totalErros++;
    digitalWrite(PINO_LED, HIGH);
}

// Saída 2: guarda os últimos avisos e erros
void saidaHistorico(int nivel, const char* mensagem) {
    historico[posHistorico] = String(nomeNivelDebug(nivel)) + ": " + mensagem;
    posHistorico = (posHistorico + 1) % 5;
}

void setup() {
    configurarDebug(DEBUG_INFO);
    mostrarTempoDebug(true);            // prefixa as linhas com o tempo desde o boot
    pinMode(PINO_LED, OUTPUT);

    adicionarSaidaDebug(saidaErros,     DEBUG_ERRO);    // só erros
    adicionarSaidaDebug(saidaHistorico, DEBUG_AVISO);   // erros e avisos

    debugInfo("Sistema pronto");
}

void loop() {
    static uint32_t ultimo = 0;
    static int ciclo = 0;
    if (millis() - ultimo < 2000) return;
    ultimo = millis();
    ciclo++;

    // Estilo printf: sem montar String
    debugInfof("Ciclo %d | tempo ligado: %lu s", ciclo, (unsigned long)(millis() / 1000));

    if (ciclo % 3 == 0) debugAvisof("Sensor demorou %d ms", 150 + ciclo);
    if (ciclo % 5 == 0) debugErro("Sensor nao respondeu");

    // Só monta o texto caro se alguém for receber
    if (debugHabilitado(DEBUG_TUDO)) {
        debugTudo("Dump completo: " + String(ciclo * 12345));
    }

    if (ciclo % 10 == 0) {
        debugInfof("---- %d erro(s) ate agora. Historico: ----", totalErros);
        for (int i = 0; i < 5; i++) {
            const String& linha = historico[(posHistorico + i) % 5];
            if (linha.length()) debugInfo("  " + linha);
        }
    }
}
