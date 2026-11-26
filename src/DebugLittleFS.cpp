/**
 * @file DebugLittleFS.cpp
 * @brief Saída do DebugManager para arquivos no LittleFS (somente ESP32).
 *
 * @author  professorThiago (https://github.com/professorThiago)
 * @license MIT
 */
#if defined(ARDUINO_ARCH_ESP32)

#include "DebugLittleFS.h"
#include <LittleFS.h>
#include <time.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

DebugLittleFS debugLittleFS;

// Letra do nível no arquivo: E=erro W=aviso I=info V=verbose T=tudo
static const char LETRAS[] = { '-', 'E', 'W', 'I', 'V', 'T' };

// -----------------------------------------------------------------------------
// Exclusão mútua do arquivo/buffer.
// Regra: NUNCA chamar debug*() com este mutex travado (evita deadlock com o
// mutex do DebugManager). Problemas internos vão direto para o buffer/Serial.
// -----------------------------------------------------------------------------
static SemaphoreHandle_t mutexArquivo = nullptr;
struct Trava {
    Trava()  { if (mutexArquivo) xSemaphoreTakeRecursive(mutexArquivo, portMAX_DELAY); }
    ~Trava() { if (mutexArquivo) xSemaphoreGiveRecursive(mutexArquivo); }
};

// =============================================================================
// Captura dos logs do ESP-IDF
//
// O vprintf do ESP-IDF roda em outras tarefas (WiFi, TLS). Aqui só se copia a
// linha para um anel em RAM; o atualizar() leva as linhas para o arquivo.
// =============================================================================

static constexpr int ANEL_LINHAS  = 16;
static constexpr int ANEL_TAMANHO = 160;
static char          anel[ANEL_LINHAS][ANEL_TAMANHO];
static volatile int  anelInicio = 0, anelFim = 0, anelPerdidas = 0;
static portMUX_TYPE  anelMux = portMUX_INITIALIZER_UNLOCKED;
static vprintf_like_t vprintfOriginal = nullptr;

static int vprintfSistema(const char* fmt, va_list args)
{
    va_list copia;
    va_copy(copia, args);
    const int r = vprintfOriginal ? vprintfOriginal(fmt, args) : vprintf(fmt, args);

    char linha[ANEL_TAMANHO];
    vsnprintf(linha, sizeof(linha), fmt, copia);
    va_end(copia);

    // Remove códigos de cor ANSI e quebras de linha
    char limpa[ANEL_TAMANHO];
    int j = 0;
    for (int i = 0; linha[i] && j < ANEL_TAMANHO - 1; i++) {
        if (linha[i] == '\033') { while (linha[i] && linha[i] != 'm') i++; if (!linha[i]) break; continue; }
        if (linha[i] == '\r' || linha[i] == '\n') continue;
        limpa[j++] = linha[i];
    }
    limpa[j] = '\0';

    // Só erros (E) e avisos (W) do sistema
    if (!(limpa[0] == 'E' || limpa[0] == 'W') || limpa[1] != ' ') return r;

    taskENTER_CRITICAL(&anelMux);
    const int prox = (anelFim + 1) % ANEL_LINHAS;
    if (prox == anelInicio) {
        anelPerdidas = anelPerdidas + 1;
    } else {
        memcpy(anel[anelFim], limpa, j + 1);
        anelFim = prox;
    }
    taskEXIT_CRITICAL(&anelMux);
    return r;
}

void DebugLittleFS::_drenarSistema()
{
    for (;;) {
        char linha[ANEL_TAMANHO];
        bool tem = false;
        int  perdidas;

        taskENTER_CRITICAL(&anelMux);
        if (anelInicio != anelFim) {
            memcpy(linha, anel[anelInicio], ANEL_TAMANHO);
            anelInicio = (anelInicio + 1) % ANEL_LINHAS;
            tem = true;
        }
        perdidas = anelPerdidas;
        anelPerdidas = 0;
        taskEXIT_CRITICAL(&anelMux);

        if (perdidas) {
            char aviso[64];
            snprintf(aviso, sizeof(aviso), "IDF: %d linha(s) do sistema perdidas", perdidas);
            _adicionarLinha('W', aviso, false);
        }
        if (!tem) break;

        String texto = String("IDF: ") + (linha + 2);
        if (linha[0] == 'E' ? DEBUG_ERRO <= _cfg.nivel : DEBUG_AVISO <= _cfg.nivel)
            _adicionarLinha(linha[0], texto.c_str(), true);
    }
}

// =============================================================================
// Inicialização
// =============================================================================

bool DebugLittleFS::iniciar(const Config& cfg)
{
    if (!mutexArquivo) mutexArquivo = xSemaphoreCreateRecursiveMutex();

    {
        Trava t;
        _cfg = cfg;
        _pendente.reserve(_cfg.tamanhoBuffer + 256);

        // Por padrão NÃO formata: o LittleFS também guarda certificados e páginas
        _ok = LittleFS.begin(_cfg.formatarSeFalhar);
        if (_ok && !LittleFS.exists(_cfg.pasta)) LittleFS.mkdir(_cfg.pasta);
        _ultimaGravacao = millis();
    }

    if (!_ok) {
        Serial.println("[DebugLittleFS] LittleFS nao montou — log em arquivo desativado.");
        return false;
    }

    if (_cfg.capturarLogsSistema && !vprintfOriginal)
        vprintfOriginal = esp_log_set_vprintf(&vprintfSistema);

    if (_idSaida < 0)
        _idSaida = adicionarSaidaDebug([this](int nivel, const char* msg) { _receber(nivel, msg); },
                                       _cfg.nivel);
    else
        definirNivelSaidaDebug(_idSaida, _cfg.nivel);

    if (_idSaida < 0) {
        Serial.println("[DebugLittleFS] Sem espaco para mais saidas no DebugManager.");
        return false;
    }

    // Linha de abertura com o motivo do reset (ajuda a investigar travamentos)
    char abertura[128];
    snprintf(abertura, sizeof(abertura), "==== BOOT | reset: %s | heap: %u ====",
             motivoReset(), (unsigned)ESP.getFreeHeap());
    _adicionarLinha('I', abertura, true);
    return true;
}

void DebugLittleFS::definirNivel(int nivel)
{
    _cfg.nivel = nivel;
    if (_idSaida >= 0) definirNivelSaidaDebug(_idSaida, nivel);
}

// =============================================================================
// Recebimento das mensagens
// =============================================================================

void DebugLittleFS::_receber(int nivel, const char* msg)
{
    if (nivel < DEBUG_ERRO || nivel > DEBUG_TUDO) return;
    _adicionarLinha(LETRAS[nivel], msg, nivel <= DEBUG_AVISO);
}

void DebugLittleFS::_adicionarLinha(char letra, const char* msg, bool gravarJa)
{
    if (!_ok) return;
    Trava t;

    // Proteção contra log descontrolado: o buffer não passa de 4x o normal
    if (_pendente.length() > _cfg.tamanhoBuffer * 4) { _descartadas++; return; }

    _pendente += _carimbo();
    _pendente += ' ';
    _pendente += letra;
    _pendente += ' ';
    _pendente += msg;
    _pendente += '\n';

    if (gravarJa || _pendente.length() >= _cfg.tamanhoBuffer) gravarAgora();
}

void DebugLittleFS::atualizar()
{
    if (!_ok) return;
    if (_cfg.capturarLogsSistema) _drenarSistema();
    Trava t;
    if (_pendente.length() && millis() - _ultimaGravacao >= _cfg.intervaloGravacaoMs) gravarAgora();
}

// =============================================================================
// Relógio
// =============================================================================

bool DebugLittleFS::horarioSincronizado() const
{
    return time(nullptr) > 1700000000;   // depois de nov/2023 = NTP respondeu
}

void DebugLittleFS::sincronizarHorario(const char* fuso, const char* s1, const char* s2)
{
    configTzTime(fuso, s1, s2);
}

String DebugLittleFS::_carimbo() const
{
    char buf[32];
    if (horarioSincronizado()) {
        time_t agora = time(nullptr);
        struct tm t;
        localtime_r(&agora, &t);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
    } else {
        const uint32_t ms = millis();
        snprintf(buf, sizeof(buf), "boot+%02lu:%02lu:%02lu.%03lu",
                 (unsigned long)(ms / 3600000UL), (unsigned long)(ms / 60000UL % 60),
                 (unsigned long)(ms / 1000UL % 60), (unsigned long)(ms % 1000));
    }
    return String(buf);
}

// =============================================================================
// Arquivos
// =============================================================================

String DebugLittleFS::_caminho(const char* nome) const
{
    return String(_cfg.pasta) + "/" + nome;
}

String DebugLittleFS::_nomeArquivo(int indice) const
{
    char nome[16];
    if (indice == 0) snprintf(nome, sizeof(nome), "atual.log");
    else             snprintf(nome, sizeof(nome), "%d.log", indice);
    return _caminho(nome);
}

void DebugLittleFS::gravarAgora()
{
    if (!_ok) return;
    Trava t;
    _ultimaGravacao = millis();
    if (_pendente.length() == 0) return;

    if (_descartadas) {
        _pendente += _carimbo() + " W DebugLittleFS: " + String(_descartadas) +
                     " linha(s) descartadas (log rapido demais)\n";
        _descartadas = 0;
    }

    File f = LittleFS.open(_nomeArquivo(0), "a");
    if (!f) { Serial.println("[DebugLittleFS] Falha ao abrir o arquivo de log."); return; }
    f.print(_pendente);
    f.close();
    _pendente = "";

    _girarSeNecessario();
}

void DebugLittleFS::_girarSeNecessario()
{
    const String atual = _nomeArquivo(0);
    if (!LittleFS.exists(atual)) return;
    File f = LittleFS.open(atual, "r");
    if (!f) return;
    const size_t tam = f.size();
    f.close();
    if (tam < _cfg.tamanhoMaxArquivo) return;

    // Remove o mais antigo e empurra os demais: 1.log → 2.log → ...
    if (_cfg.arquivosAntigos == 0) { LittleFS.remove(atual); return; }
    const String maisAntigo = _nomeArquivo(_cfg.arquivosAntigos);
    if (LittleFS.exists(maisAntigo)) LittleFS.remove(maisAntigo);
    for (int i = _cfg.arquivosAntigos - 1; i >= 1; i--)
        if (LittleFS.exists(_nomeArquivo(i))) LittleFS.rename(_nomeArquivo(i), _nomeArquivo(i + 1));
    LittleFS.rename(atual, _nomeArquivo(1));
}

void DebugLittleFS::paraCadaLinha(uint16_t n, const std::function<void(const String&)>& cb)
{
    if (!_ok || n == 0) return;
    String* linhas = new String[n];
    uint16_t total = 0, pos = 0;
    const size_t janela = (size_t)n * 120 + 120;   // ~120 bytes por linha

    // Lê só o final do arquivo, guardando as últimas n linhas num anel
    auto lerFinal = [&](const String& caminho) {
        if (!LittleFS.exists(caminho)) return;
        File f = LittleFS.open(caminho, "r");
        if (!f) return;
        const size_t inicio = f.size() > janela ? f.size() - janela : 0;
        f.seek(inicio);
        if (inicio > 0) f.readStringUntil('\n');   // descarta a linha cortada
        while (f.available()) {
            String l = f.readStringUntil('\n');
            if (l.length() == 0) continue;
            linhas[pos] = l;
            pos = (pos + 1) % n;
            if (total < n) total++;
        }
        f.close();
    };

    {
        Trava t;
        gravarAgora();
        // Se o atual.log é pequeno (acabou de girar), completa com o 1.log
        size_t tamAtual = 0;
        if (LittleFS.exists(_nomeArquivo(0))) {
            File f = LittleFS.open(_nomeArquivo(0), "r");
            if (f) { tamAtual = f.size(); f.close(); }
        }
        if (tamAtual < janela && _cfg.arquivosAntigos > 0) lerFinal(_nomeArquivo(1));
        lerFinal(_nomeArquivo(0));
    }

    // O callback roda FORA da trava: pode publicar por MQTT e logar à vontade
    const uint16_t primeiro = (total < n) ? 0 : pos;
    for (uint16_t i = 0; i < total; i++) cb(linhas[(primeiro + i) % n]);
    delete[] linhas;
}

String DebugLittleFS::ultimasLinhas(uint16_t n)
{
    String saida;
    paraCadaLinha(n, [&saida](const String& l) { saida += l; saida += '\n'; });
    return saida;
}

void DebugLittleFS::imprimirTudo(Print& saida)
{
    if (!_ok) return;
    Trava t;
    gravarAgora();
    for (int i = _cfg.arquivosAntigos; i >= 0; i--) {
        const String nome = _nomeArquivo(i);
        if (!LittleFS.exists(nome)) continue;
        File f = LittleFS.open(nome, "r");
        if (!f) continue;
        saida.printf("===== %s (%u bytes) =====\n", nome.c_str(), (unsigned)f.size());
        uint8_t buf[256];
        while (f.available()) {
            const size_t n = f.read(buf, sizeof(buf));
            saida.write(buf, n);
        }
        f.close();
    }
}

bool DebugLittleFS::limpar()
{
    if (!_ok) return false;
    Trava t;
    _pendente = "";
    bool ok = true;
    for (int i = 0; i <= _cfg.arquivosAntigos; i++)
        if (LittleFS.exists(_nomeArquivo(i))) ok &= LittleFS.remove(_nomeArquivo(i));
    return ok;
}

size_t DebugLittleFS::tamanhoTotal()
{
    if (!_ok) return 0;
    Trava t;
    size_t total = _pendente.length();
    for (int i = 0; i <= _cfg.arquivosAntigos; i++) {
        if (!LittleFS.exists(_nomeArquivo(i))) continue;
        File f = LittleFS.open(_nomeArquivo(i), "r");
        if (f) { total += f.size(); f.close(); }
    }
    return total;
}

// =============================================================================
// Motivo do reset
// =============================================================================

const char* DebugLittleFS::motivoReset()
{
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:   return "energia ligada";
        case ESP_RST_EXT:       return "pino de reset";
        case ESP_RST_SW:        return "reinicio por software";
        case ESP_RST_PANIC:     return "PANICO (excecao/travamento)";
        case ESP_RST_INT_WDT:   return "WATCHDOG de interrupcao";
        case ESP_RST_TASK_WDT:  return "WATCHDOG de tarefa (loop travado)";
        case ESP_RST_WDT:       return "WATCHDOG";
        case ESP_RST_DEEPSLEEP: return "saida do deep sleep";
        case ESP_RST_BROWNOUT:  return "BROWNOUT (queda de tensao)";
        case ESP_RST_SDIO:      return "SDIO";
        default:                return "desconhecido";
    }
}

#endif // ARDUINO_ARCH_ESP32
