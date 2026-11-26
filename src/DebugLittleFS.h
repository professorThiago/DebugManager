/**
 * @file DebugLittleFS.h
 * @brief Grava as mensagens do DebugManager em arquivos no LittleFS (ESP32).
 *
 * Funciona como uma saída extra do DebugManager: tudo o que passa por
 * debugErro(), debugAviso(), debugInfo()... — inclusive as mensagens das
 * bibliotecas que usam o DebugManager, como a ConectividadeESP32 — vai
 * também para o arquivo, com data e hora.
 *
 * @code
 * #include <DebugManager.h>
 * #include <DebugLittleFS.h>
 *
 * void setup() {
 *     configurarDebug(DEBUG_INFO);
 *     debugLittleFS.iniciar();                    // grava AVISO e ERRO (padrão)
 * }
 * void loop() {
 *     debugLittleFS.atualizar();                  // grava o buffer na flash
 * }
 * @endcode
 *
 * Formato:   2026-11-26 14:03:12 E MQTT: falha na conexao! Codigo: -2
 *            boot+00:00:07.120   W WiFi: timeout (15s). Falha 1.
 *
 * Arquivos:  /logs/atual.log, /logs/1.log, /logs/2.log ... (rotação por tamanho)
 *
 * Cuidados com a flash: as mensagens ficam num buffer em RAM e vão para o
 * arquivo a cada `intervaloGravacaoMs`; erros e avisos são gravados na hora
 * (se o ESP32 travar, eles já estão salvos).
 *
 * @author  professorThiago (https://github.com/professorThiago)
 * @license MIT
 */
#pragma once

#if defined(ARDUINO_ARCH_ESP32)

#include <Arduino.h>
#include <functional>
#include "DebugManager.h"

class DebugLittleFS
{
public:
    struct Config {
        int         nivel               = DEBUG_AVISO;  ///< Nível gravado no arquivo (independente do Serial)
        const char* pasta               = "/logs";
        size_t      tamanhoMaxArquivo   = 64 * 1024;    ///< Bytes por arquivo antes de girar
        uint8_t     arquivosAntigos     = 2;            ///< Além do atual.log
        uint32_t    intervaloGravacaoMs = 30000;        ///< Grava o buffer a cada 30 s...
        size_t      tamanhoBuffer       = 2048;         ///< ...ou quando passar disso
        bool        capturarLogsSistema = true;         ///< Erros/avisos do ESP-IDF (WiFi, TLS)
        bool        formatarSeFalhar    = false;        ///< Formata o LittleFS se não montar (apaga tudo!)
    };

    /** @brief Monta o LittleFS e registra o arquivo como saída extra do DebugManager. */
    bool iniciar(const Config& cfg);
    bool iniciar(int nivel = DEBUG_AVISO) { Config c; c.nivel = nivel; return iniciar(c); }

    /** @brief Chame em todo loop(): grava o buffer e traz os logs do sistema. */
    void atualizar();

    /** @brief Grava o buffer agora (chame antes de ESP.restart() ou deep sleep). */
    void gravarAgora();

    /** @brief Muda o nível gravado no arquivo. */
    void definirNivel(int nivel);
    int  obterNivel() const      { return _cfg.nivel; }

    /** @brief Sincroniza o relógio por NTP (chame quando o WiFi conectar). */
    void sincronizarHorario(const char* fusoPosix = "<-03>3",
                            const char* servidor1 = "pool.ntp.org",
                            const char* servidor2 = "time.google.com");
    bool horarioSincronizado() const;

    /** @brief Últimas N linhas do arquivo atual. */
    String ultimasLinhas(uint16_t n);
    /** @brief Percorre as últimas N linhas (útil para enviar em partes por MQTT). */
    void   paraCadaLinha(uint16_t n, const std::function<void(const String&)>& cb);
    /** @brief Imprime todos os arquivos, do mais antigo ao atual. */
    void   imprimirTudo(Print& saida);
    /** @brief Apaga os arquivos de log. */
    bool   limpar();
    /** @brief Bytes ocupados pelos logs. */
    size_t tamanhoTotal();
    /** @brief `true` se o LittleFS montou e o log está gravando. */
    bool   ativo() const { return _ok; }

    /** @brief Texto do motivo do último reset (queda de energia, watchdog, pânico...). */
    static const char* motivoReset();

    // ---------------- English API ----------------
    bool   begin(const Config& cfg)                    { return iniciar(cfg); }
    bool   begin(int level = DEBUG_WARNING)            { return iniciar(level); }
    void   update()                                    { atualizar(); }
    void   flush()                                     { gravarAgora(); }
    void   setLevel(int level)                         { definirNivel(level); }
    int    getLevel() const                            { return obterNivel(); }
    void   syncTime(const char* tz = "<-03>3",
                    const char* s1 = "pool.ntp.org",
                    const char* s2 = "time.google.com") { sincronizarHorario(tz, s1, s2); }
    bool   isTimeSynced() const                        { return horarioSincronizado(); }
    String lastLines(uint16_t n)                       { return ultimasLinhas(n); }
    void   forEachLine(uint16_t n, const std::function<void(const String&)>& cb) { paraCadaLinha(n, cb); }
    void   printAll(Print& out)                        { imprimirTudo(out); }
    bool   clear()                                     { return limpar(); }
    size_t totalSize()                                 { return tamanhoTotal(); }
    bool   isActive() const                            { return ativo(); }
    static const char* resetReason()                   { return motivoReset(); }

private:
    Config   _cfg;
    bool     _ok = false;
    int      _idSaida = -1;
    String   _pendente;
    uint32_t _ultimaGravacao = 0;
    uint32_t _descartadas = 0;

    void   _receber(int nivel, const char* msg);
    void   _adicionarLinha(char letra, const char* msg, bool gravarJa);
    String _caminho(const char* nome) const;
    String _nomeArquivo(int indice) const;
    String _carimbo() const;
    void   _girarSeNecessario();
    void   _drenarSistema();
};

extern DebugLittleFS debugLittleFS;

#endif // ARDUINO_ARCH_ESP32
