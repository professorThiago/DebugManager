/**
 * @file 04_EnglishAPI.ino
 * @brief DebugManager — Example 4: English API + file log (ESP32).
 *
 * Every function has an English name that calls the same code:
 *   configureDebug = configurarDebug, debugError = debugErro,
 *   debugWarning = debugAviso, debugTrace = debugTudo,
 *   addDebugOutput = adicionarSaidaDebug ...
 * debugInfo() and debugVerbose() are the same in both languages.
 *
 * @author  professorThiago (https://github.com/professorThiago)
 * @license MIT
 */

#include <Arduino.h>
#include <DebugManager.h>
#include <DebugLittleFS.h>

int errorCount = 0;

void setup() {
    configureDebug(DEBUG_INFO, 4);      // INFO level; GPIO 4 tied to GND at boot → DEBUG_TRACE
    showDebugTime(true);

    debugLittleFS.begin(DEBUG_WARNING); // warnings and errors go to /logs/atual.log

    addDebugOutput([](int level, const char* msg) { errorCount++; }, DEBUG_ERROR);

    debugInfo("Ready");
    debugInfof("Reset reason: %s", DebugLittleFS::resetReason());
}

void loop() {
    debugLittleFS.update();

    static uint32_t last = 0;
    static int n = 0;
    if (millis() - last < 3000) return;
    last = millis();
    n++;

    debugInfof("Tick %d", n);
    if (n % 3 == 0) debugWarningf("Sensor slow (%d ms)", 200 + n);
    if (n % 5 == 0) debugError("Sensor timeout");
    if (isDebugEnabled(DEBUG_TRACE)) debugTrace("Raw: " + String(n * 97));

    if (n % 10 == 0) {
        debugInfof("%d error(s) so far. Last lines in the file:", errorCount);
        debugLittleFS.forEachLine(5, [](const String& line) { Serial.println("  " + line); });
    }
}
