// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "wasapi.hpp"
#include "pcm.hpp"
#include "dsp.hpp"
#include "ai_speech.hpp"
#include <QThread>
#include <deque>
#include <functional>
#include <memory>
#include <chrono>

class WindowsRender {
    Wasapi::Ptr<IAudioClient> client;
    Wasapi::Ptr<IAudioRenderClient> render;
    Wasapi::Ptr<IAudioClockAdjustment> clock;
    std::deque<float> queue;
    UINT32 capacity = 0, prebuffer = 0, rate = 48000;
    bool running = false;
    std::chrono::steady_clock::time_point adjusted{};
public:
    WindowsRender(IMMDeviceEnumerator *enumerator, const QString &id, unsigned sampleRate, bool monitor)
        : rate(sampleRate) {
        Wasapi::Ptr<IMMDevice> device; Wasapi::device(enumerator, id, device);
        Wasapi::activate(device.get(), client);
        WAVEFORMATEX format{};
        format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT; format.nChannels = 1;
        format.nSamplesPerSec = rate; format.wBitsPerSample = 32;
        format.nBlockAlign = sizeof(float); format.nAvgBytesPerSec = rate * sizeof(float);
        const DWORD flags = AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY |
                            AUDCLNT_STREAMFLAGS_RATEADJUST | AUDCLNT_STREAMFLAGS_NOPERSIST;
        Wasapi::check(client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, monitor ? 1600000 : 1200000,
                                        0, &format, nullptr), "Configurar saída");
        Wasapi::check(client->GetBufferSize(&capacity), "Ler buffer da saída");
        Wasapi::check(client->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void **>(render.put())), "Preparar saída");
        Wasapi::check(client->GetService(__uuidof(IAudioClockAdjustment), reinterpret_cast<void **>(clock.put())), "Sincronizar saída");
        prebuffer = std::min(UINT32(rate * (monitor ? .096 : .064)), capacity);
    }
    void append(const std::vector<float> &samples) {
        queue.insert(queue.end(), samples.begin(), samples.end());
        while (queue.size() > rate / 2) queue.pop_front();
    }
    void write(float gain) {
        UINT32 padding = 0; Wasapi::check(client->GetCurrentPadding(&padding), "Verificar saída");
        // Rebuffer on an actual underrun instead of playing intermittent short blocks.
        if (running && padding == 0) {
            Wasapi::check(client->Stop(), "Pausar saída");
            Wasapi::check(client->Reset(), "Reiniciar buffer"); running = false;
        }
        if (!running && queue.size() < prebuffer) return;
        const UINT32 frames = std::min<UINT32>(capacity - padding, UINT32(queue.size()));
        if (frames) {
            BYTE *buffer = nullptr;
            Wasapi::check(render->GetBuffer(frames, &buffer), "Obter buffer de saída");
            auto *samples = reinterpret_cast<float *>(buffer);
            for (UINT32 i = 0; i < frames; ++i) { samples[i] = queue.front() * gain; queue.pop_front(); }
            Wasapi::check(render->ReleaseBuffer(frames, 0), "Enviar áudio");
        }
        if (!running) { Wasapi::check(client->Start(), "Iniciar saída"); running = true; }
        const auto now = std::chrono::steady_clock::now();
        if (now - adjusted > std::chrono::milliseconds(250)) {
            // Independent USB, cable and headphone clocks drift. Adjust playback by
            // at most 0.5%, rather than periodically discarding chunks of speech.
            const double occupancy = double(padding + frames + queue.size());
            const double correction = std::clamp((occupancy - prebuffer) / (rate * 4.0), -.005, .005);
            Wasapi::check(clock->SetSampleRate(float(rate * (1.0 + correction))), "Sincronizar relógio da saída");
            adjusted = now;
        }
    }
    ~WindowsRender() { if (client && running) client->Stop(); }
};

class AudioWorker : public QThread {
    Controls &controls;
    QString inputName, outputName, busName;
    AiSettings aiSettings;
protected:
    void run() override {
        try {
            Wasapi::Apartment apartment;
            Wasapi::Ptr<IMMDeviceEnumerator> enumerator; Wasapi::enumerator(enumerator);
            Wasapi::Ptr<IMMDevice> microphone; Wasapi::device(enumerator.get(), inputName, microphone);
            Wasapi::Ptr<IAudioClient> capture; Wasapi::activate(microphone.get(), capture);
            WAVEFORMATEX *rawFormat = nullptr;
            Wasapi::check(capture->GetMixFormat(&rawFormat), "Ler formato do microfone");
            std::unique_ptr<WAVEFORMATEX, decltype(&CoTaskMemFree)> format(rawFormat, CoTaskMemFree);
            bool floating = format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT;
            if (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
                const auto *ext = reinterpret_cast<const WAVEFORMATEXTENSIBLE *>(format.get());
                floating = IsEqualGUID(ext->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
                if (!floating && !IsEqualGUID(ext->SubFormat, KSDATAFORMAT_SUBTYPE_PCM))
                    throw std::runtime_error("Selecione um microfone com formato PCM no Windows.");
            } else if (!floating && format->wFormatTag != WAVE_FORMAT_PCM)
                throw std::runtime_error("Formato de captura não suportado.");
            unsigned char test[8]{}; pcmSample(test, format->wBitsPerSample, floating);
            const auto rate = format->nSamplesPerSec;
            const auto channels = format->nChannels;
            if (rate < 8000 || rate > 192000 || channels == 0 || channels > 32 ||
                format->nBlockAlign < channels * (format->wBitsPerSample / 8))
                throw std::runtime_error("Formato do microfone inválido.");
            Wasapi::check(capture->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_NOPERSIST,
                                              1000000, 0, format.get(), nullptr), "Configurar microfone");
            Wasapi::Ptr<IAudioCaptureClient> reader;
            Wasapi::check(capture->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void **>(reader.put())), "Preparar captura");
            Processor dsp(rate);
            WindowsRender output(enumerator.get(), busName, rate, false);
            std::unique_ptr<WindowsRender> monitor;
            std::vector<float> pending, processed(dsp.block); pending.reserve(dsp.block);
            Wasapi::check(capture->Start(), "Iniciar microfone");
            std::unique_ptr<AiSpeech> ai;
            if (aiSettings.enabled) ai = std::make_unique<AiSpeech>(aiSettings, rate, onAiStatus);
            if (onReady) onReady();
            while (!isInterruptionRequested()) {
                if (controls.listen && !monitor) {
                    if (outputName.isEmpty() || outputName == busName)
                        throw std::runtime_error("Selecione seus fones como saída de escuta.");
                    monitor = std::make_unique<WindowsRender>(enumerator.get(), outputName, rate, true);
                } else if (!controls.listen && monitor) monitor.reset();
                UINT32 packet = 0;
                Wasapi::check(reader->GetNextPacketSize(&packet), "Ler microfone");
                while (packet && !isInterruptionRequested()) {
                    BYTE *data = nullptr; UINT32 frames = 0; DWORD flags = 0;
                    Wasapi::check(reader->GetBuffer(&data, &frames, &flags, nullptr, nullptr), "Obter áudio do microfone");
                    for (UINT32 i = 0; i < frames; ++i) {
                        float mono = 0;
                        if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && data) {
                            for (unsigned c = 0; c < channels; ++c)
                                mono += pcmSample(data + i * format->nBlockAlign + c * (format->wBitsPerSample / 8),
                                                  format->wBitsPerSample, floating);
                            mono /= channels;
                        }
                        pending.push_back(mono);
                        if (pending.size() == dsp.block) {
                            if (ai) ai->processBlock(pending.data(), processed.data(), pending.size(), controls);
                            else dsp.process(pending.data(), processed.data(), controls);
                            output.append(processed); if (monitor) monitor->append(processed);
                            pending.clear();
                        }
                    }
                    Wasapi::check(reader->ReleaseBuffer(frames), "Liberar captura");
                    output.write(1.f); if (monitor) monitor->write(controls.listenVolume / 100.f);
                    Wasapi::check(reader->GetNextPacketSize(&packet), "Ler microfone");
                }
                output.write(1.f); if (monitor) monitor->write(controls.listenVolume / 100.f);
                msleep(2);
            }
            capture->Stop();
        } catch (const std::exception &e) {
            if (!isInterruptionRequested()) error = QString::fromUtf8(e.what());
        }
        controls.level = 0;
    }
public:
    QString error;
    std::function<void()> onReady;
    std::function<void(QString)> onAiStatus;
    AudioWorker(Controls &c, QString input, QString headphone, QString bus, AiSettings ai = {})
        : controls(c), inputName(input), outputName(headphone), busName(bus), aiSettings(std::move(ai)) {}
    ~AudioWorker() override { requestInterruption(); wait(); }
};
