// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "dsp.hpp"
#include "ai_speech.hpp"
#include <QThread>
#include <QString>
#include <pulse/pulseaudio.h>
#include <pulse/util.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <chrono>
#include <deque>
#include <functional>
#include <stdexcept>
#ifndef JOCAS_AUDIO_CLIENT_NAME
#define JOCAS_AUDIO_CLIENT_NAME "Modificador de Voz JOCAS"
#endif

class AudioWorker : public QThread {
    pa_mainloop *loop = nullptr;
    pa_context *context = nullptr;
    pa_stream *capture = nullptr, *output = nullptr, *monitor = nullptr;
    Controls &controls;
    QString inputName, outputName, busName;
    AiSettings aiSettings;
    Processor dsp;
    std::vector<float> pending, processed, transfer;
    std::deque<float> sendQueue, listenQueue;
    bool listening = false;
    void check(bool ok, const char *what) {
        if (!ok) throw std::runtime_error(std::string(what) + ": " +
            (context ? pa_strerror(pa_context_errno(context)) : "sem contexto de áudio"));
    }
    void pump() {
        int result = 0;
        check(pa_mainloop_iterate(loop, 0, &result) >= 0, "Conexão de áudio perdida");
        const auto state = pa_context_get_state(context);
        check(PA_CONTEXT_IS_GOOD(state), "Servidor de áudio desconectado");
    }
    template<class F> void until(F ready) {
        auto start = std::chrono::steady_clock::now();
        while (!ready()) {
            if (isInterruptionRequested()) throw std::runtime_error("Inicialização cancelada");
            pump();
            if (std::chrono::steady_clock::now() - start > std::chrono::seconds(6))
                throw std::runtime_error("O servidor de áudio não respondeu em 6 segundos.");
            msleep(1);
        }
    }
    pa_stream *stream(const char *title, const QString &device, bool recording, bool monitoring = false) {
        pa_sample_spec spec{PA_SAMPLE_FLOAT32NE, 48000, 1};
        pa_stream *s = pa_stream_new(context, title, &spec, nullptr);
        check(s != nullptr, "Não foi possível criar o fluxo");
        pa_buffer_attr attr;
        // Keep a 200 ms server-side reserve on BOTH destinations. Games can
        // delay this client for >100 ms, even when the audio server stays on time.
        // Do not use ADJUST_LATENCY on playback: it would include hardware latency
        // in this target and shrink the actual per-stream safety reserve.
        attr.maxlength = 24000 * sizeof(float);
        attr.tlength = 9600 * sizeof(float);
        attr.prebuf = 7680 * sizeof(float);
        attr.minreq = 512 * sizeof(float);
        attr.fragsize = 512 * sizeof(float);
        const auto flags = pa_stream_flags_t(PA_STREAM_DONT_MOVE |
            (monitoring ? PA_STREAM_START_CORKED : recording ? PA_STREAM_ADJUST_LATENCY : PA_STREAM_NOFLAGS));
        int result = recording ? pa_stream_connect_record(s, device.toUtf8().constData(), &attr, flags)
            : pa_stream_connect_playback(s, device.toUtf8().constData(), &attr, flags, nullptr, nullptr);
        if (result < 0) { pa_stream_unref(s); check(false, "Dispositivo indisponível"); }
        return s;
    }
    static void release(pa_stream *&s) {
        if (s) { pa_stream_disconnect(s); pa_stream_unref(s); s = nullptr; }
    }
    void queue(std::deque<float> &q, const std::vector<float> &samples) {
        q.insert(q.end(), samples.begin(), samples.end());
        // Accommodate capture catch-up after a scheduling stall. The former
        // 100 ms limit discarded words in a single delayed capture fragment.
        // Still bound recovery latency if a device stops consuming audio.
        while (q.size() > 24000) q.pop_front();
    }
    void write(pa_stream *s, std::deque<float> &q, float gain) {
        if (!s || pa_stream_get_state(s) != PA_STREAM_READY || q.empty()) return;
        const size_t writable = pa_stream_writable_size(s);
        check(writable != size_t(-1), "Saída indisponível");
        size_t n = std::min(q.size(), writable / sizeof(float));
        if (!n) return;
        transfer.resize(n);
        for (size_t i = 0; i < n; ++i) { transfer[i] = q.front() * gain; q.pop_front(); }
        check(pa_stream_write(s, transfer.data(), n * sizeof(float), nullptr, 0, PA_SEEK_RELATIVE) >= 0,
              "Falha ao enviar áudio");
    }
protected:
    void run() override {
        try {
            loop = pa_mainloop_new();
            check(loop != nullptr, "Falha ao iniciar áudio");
            context = pa_context_new(pa_mainloop_get_api(loop), JOCAS_AUDIO_CLIENT_NAME);
            check(context != nullptr, "Falha ao iniciar áudio");
            check(pa_context_connect(context, nullptr, PA_CONTEXT_NOAUTOSPAWN, nullptr) >= 0,
                  "Não foi possível conectar ao PipeWire/PulseAudio");
            until([&] { return pa_context_get_state(context) == PA_CONTEXT_READY; });
            output = stream("Voz modificada → JOCAS Microfone", busName, false);
            capture = stream("Microfone → efeitos", inputName, true);
            until([&] {
                check(PA_STREAM_IS_GOOD(pa_stream_get_state(capture)) && PA_STREAM_IS_GOOD(pa_stream_get_state(output)),
                      "Microfone ou saída indisponível");
                return pa_stream_get_state(capture) == PA_STREAM_READY && pa_stream_get_state(output) == PA_STREAM_READY;
            });
            std::unique_ptr<AiSpeech> ai;
            if (aiSettings.enabled) ai = std::make_unique<AiSpeech>(aiSettings, 48000, onAiStatus);
            // Boost only this worker, never the UI, games or audio server.
            // libpulse negotiates the available RT policy without requiring sudo.
            // If unavailable, request a fair-scheduler boost and keep working
            // normally even if that request is denied too.
            if (pa_thread_make_realtime(5) < 0)
                (void)setpriority(PRIO_PROCESS, id_t(syscall(SYS_gettid)), -10);
            if (onReady) onReady();
            processed.resize(dsp.block); pending.reserve(dsp.block);
            while (!isInterruptionRequested()) {
                pump();
                check(pa_stream_get_state(capture) == PA_STREAM_READY && pa_stream_get_state(output) == PA_STREAM_READY,
                      "Microfone desconectado. Reconecte e inicie novamente");
                bool wantListen = controls.listen;
                if (wantListen && !monitor) {
                    if (outputName.isEmpty()) throw std::runtime_error("Selecione uma saída para escutar o microfone.");
                    monitor = stream("Escutar meu microfone", outputName, false, true);
                }
                if (monitor) {
                    check(PA_STREAM_IS_GOOD(pa_stream_get_state(monitor)), "Saída de escuta desconectada");
                    if (pa_stream_get_state(monitor) == PA_STREAM_READY && wantListen != listening) {
                        auto *op = pa_stream_flush(monitor, nullptr, nullptr); if (op) pa_operation_unref(op);
                        op = pa_stream_cork(monitor, wantListen ? 0 : 1, nullptr, nullptr); if (op) pa_operation_unref(op);
                        listenQueue.clear(); listening = wantListen;
                    }
                }
                size_t available = pa_stream_readable_size(capture);
                check(available != size_t(-1), "Falha ao ler microfone");
                // Drain queued fragments instead of sleeping after every peek.
                // Bound each batch so stop/mute/monitor changes remain responsive.
                for (int fragment = 0; available && fragment < 32; ++fragment) {
                    const void *data = nullptr; size_t bytes = 0;
                    check(pa_stream_peek(capture, &data, &bytes) >= 0, "Falha ao ler microfone");
                    const float *samples = static_cast<const float *>(data);
                    for (size_t i = 0; i < bytes / sizeof(float); ++i) {
                        pending.push_back(samples ? samples[i] : 0.f);
                        if (pending.size() == dsp.block) {
                            if (ai) ai->processBlock(pending.data(), processed.data(), pending.size(), controls);
                            else dsp.process(pending.data(), processed.data(), controls);
                            queue(sendQueue, processed);
                            if (listening) queue(listenQueue, processed);
                            // Feed playback during catch-up, before the next block
                            // can fill the client queue. Do not truncate a burst.
                            write(output, sendQueue, 1.f);
                            if (listening) write(monitor, listenQueue, controls.listenVolume / 100.f);
                            pending.clear();
                        }
                    }
                    if (bytes) check(pa_stream_drop(capture) >= 0, "Falha ao liberar áudio");
                    else break;
                    available = pa_stream_readable_size(capture);
                    check(available != size_t(-1), "Falha ao ler microfone");
                }
                write(output, sendQueue, 1.f);
                if (listening) write(monitor, listenQueue, controls.listenVolume / 100.f);
                msleep(1);
            }
        } catch (const std::exception &e) {
            if (!isInterruptionRequested()) error = QString::fromUtf8(e.what());
        }
        release(capture); release(monitor); release(output);
        if (context) { pa_context_disconnect(context); pa_context_unref(context); context = nullptr; }
        if (loop) { pa_mainloop_free(loop); loop = nullptr; }
        controls.level = 0;
    }
public:
    QString error;
    std::function<void()> onReady;
    std::function<void(QString)> onAiStatus;
    AudioWorker(Controls &c, QString input, QString headphone, QString bus, AiSettings ai = {})
        : controls(c), inputName(input), outputName(headphone), busName(bus), aiSettings(std::move(ai)) {
        setObjectName("JOCAS-audio");
    }
    ~AudioWorker() override { requestInterruption(); wait(); }
};
