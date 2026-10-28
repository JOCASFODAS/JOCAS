// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "dsp.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <deque>
#include <functional>
#include <memory>

namespace AiPaths {
inline QString root() { return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/Gepetas/ia"; }
inline QString helper() { return QCoreApplication::applicationDirPath() + "/ai/worker.py"; }
inline QString python() {
#ifdef Q_OS_WIN
    return root() + "/venv/Scripts/python.exe";
#else
    return root() + "/venv/bin/python";
#endif
}
inline QString uv() {
#ifdef Q_OS_WIN
    return QCoreApplication::applicationDirPath() + "/ai/tools/uv.exe";
#else
    return QCoreApplication::applicationDirPath() + "/ai/tools/uv";
#endif
}
inline QProcessEnvironment environment() {
    auto env = QProcessEnvironment::systemEnvironment();
    env.insert("UV_CACHE_DIR", root() + "/cache");
    env.insert("UV_PYTHON_INSTALL_DIR", root() + "/python");
    env.insert("PYTHONUTF8", "1"); env.insert("PYTHONUNBUFFERED", "1");
    env.insert("HF_HUB_DISABLE_TELEMETRY", "1");
    return env;
}
}
struct AiSettings {
    bool enabled = false;
    QJsonObject config;
    QString key;
    QString program; // Test override; never exposed to imported models or settings.
    QStringList arguments;
};

// Lives exclusively on the native audio thread. Inference is isolated in a child
// process, so model loading, network waits and synthesis never block audio writes.
class AiSpeech {
    QProcess process;
    QJsonObject config;
    unsigned rate;
    std::function<void(QString)> status;
    QByteArray lines;
    std::vector<float> phrase;
    std::deque<float> preroll, output;
    struct Request { QByteArray pcm; int epoch; };
    std::deque<Request> requests;
    bool ready = false, busy = false, failed = false, wasMuted = false;
    int epoch = 0;
    size_t quiet = 0, voiced = 0;
    float threshold;
    QString lastStatus;
    void tell(const QString &s) { if (s != lastStatus) { lastStatus = s; if (status) status(s); } }
    void resetCapture() { phrase.clear(); preroll.clear(); quiet = voiced = 0; }
    void invalidate() { ++epoch; requests.clear(); output.clear(); resetCapture(); }
    static QByteArray pcm16(const std::vector<float> &samples, unsigned inRate) {
        const size_t count = samples.size() * 16000ull / inRate;
        QByteArray bytes; bytes.resize(qsizetype(count * 2));
        for (size_t i = 0; i < count; ++i) {
            const double pos = i * double(inRate) / 16000.;
            const size_t j = std::min(size_t(pos), samples.size() - 1), k = std::min(j + 1, samples.size() - 1);
            const float v = samples[j] + (samples[k] - samples[j]) * float(pos - j);
            const int x = int(std::clamp(v, -.999f, .999f) * 32767);
            bytes[qsizetype(2*i)] = char(x & 255); bytes[qsizetype(2*i+1)] = char((x >> 8) & 255);
        }
        return bytes.toBase64();
    }
    void sendNext() {
        if (!ready || busy || failed || requests.empty()) return;
        const auto r = requests.front(); requests.pop_front();
        const QJsonObject o{{"pcm", QString::fromLatin1(r.pcm)}, {"epoch", r.epoch}};
        process.write(QJsonDocument(o).toJson(QJsonDocument::Compact) + '\n');
        busy = true; tell("IA processando sua frase…");
    }
    void finishPhrase() {
        if (voiced >= rate / 5 && !phrase.empty()) {
            if (requests.size() < 2) requests.push_back({pcm16(phrase, rate), epoch});
            else tell("Fila de IA cheia. Aguarde a voz terminar antes de falar novamente.");
        }
        resetCapture(); sendNext();
    }
    void acceptAudio(const QJsonObject &o) {
        if (o.value("epoch").toInt(-1) != epoch || wasMuted) return;
        const unsigned sampleRate = o.value("rate").toInt();
        const auto data = QByteArray::fromBase64(o.value("pcm").toString().toLatin1());
        if (sampleRate < 8000 || sampleRate > 96000 || data.size() % 2 || data.size() > qint64(sampleRate) * 60) {
            tell("A IA devolveu um formato de áudio inválido."); return;
        }
        const size_t n = size_t(data.size()) / 2;
        if (!n) { tell("IA não reconheceu fala nessa frase. Tente falar mais perto do microfone."); return; }
        const size_t count = n * uint64_t(rate) / sampleRate;
        if (output.size() + count > rate * 30ull) { tell("Fila de voz cheia; aguarde a reprodução terminar."); return; }
        auto at = [&](size_t i) { return float(qint16(quint8(data[qsizetype(i*2)]) | (quint16(quint8(data[qsizetype(i*2+1)])) << 8))) / 32768.f; };
        for (size_t i = 0; i < count; ++i) {
            const double p = i * double(sampleRate) / rate;
            const size_t j = std::min(size_t(p), n-1), k = std::min(j+1,n-1);
            output.push_back(at(j) + (at(k) - at(j)) * float(p-j));
        }
        tell("Reproduzindo voz de IA • faça pausas entre as frases.");
    }
    void poll() {
        if (failed) return;
        process.waitForReadyRead(0);
        process.readAllStandardError(); // Never publish libraries' logs or phrase text.
        lines += process.readAllStandardOutput();
        if (lines.size() > 12 * 1024 * 1024) { failed = true; invalidate(); tell("Resposta da IA excedeu o limite."); process.kill(); return; }
        qsizetype end;
        while ((end = lines.indexOf('\n')) >= 0) {
            const auto o = QJsonDocument::fromJson(lines.left(end)).object(); lines.remove(0,end+1);
            const auto type = o.value("type").toString();
            if (type == "ready") { ready = true; tell(o.value("message").toString()); }
            else if (type == "audio") { busy = false; acceptAudio(o); }
            else if (type == "error") { busy = false; if (o.value("epoch").toInt(-1) == epoch) tell(o.value("message").toString()); }
            else if (type == "fatal") { failed = true; invalidate(); tell(o.value("message").toString()); }
        }
        if (process.state() == QProcess::NotRunning && !failed) {
            failed = true; invalidate(); tell("IA parou. A saída ficou em silêncio. Pare o microfone, confira o preparo e inicie novamente.");
        }
        sendNext();
    }
public:
    explicit AiSpeech(const AiSettings &s, unsigned sampleRate, std::function<void(QString)> callback = {})
        : config(s.config), rate(sampleRate), status(std::move(callback)),
          threshold(std::pow(10.f, float(config.value("sensitivity").toInt(-42)) / 20.f)) {
        auto env = AiPaths::environment();
        env.insert("JOCAS_OPENAI_API_KEY", s.key);
        process.setProcessEnvironment(env);
        process.start(s.program.isEmpty() ? AiPaths::python() : s.program,
            s.program.isEmpty() ? QStringList{AiPaths::helper(), "serve"} : s.arguments);
        process.write(QJsonDocument(config).toJson(QJsonDocument::Compact) + '\n');
        tell("Carregando IA… A saída ficará em silêncio até a primeira frase gerada.");
    }
    ~AiSpeech() {
        process.closeWriteChannel();
        if (process.state() != QProcess::NotRunning) { process.kill(); process.waitForFinished(2000); }
    }
    void processBlock(const float *input, float *out, size_t count, Controls &controls) {
        const bool muted = controls.mute;
        if (muted != wasMuted) {
            invalidate(); wasMuted = muted;
            tell(muted ? "IA silenciada. Frases pendentes foram descartadas." : "IA reativada. Fale uma nova frase.");
        }
        poll();
        std::fill(out, out + count, 0.f);
        if (muted || failed || !ready) { controls.level = 0; return; }
        float energy = 0;
        for (size_t i=0;i<count;++i) energy += input[i] * input[i];
        const bool speech = std::sqrt(energy / count) >= threshold;
        if (speech || !phrase.empty()) {
            if (phrase.empty()) { phrase.assign(preroll.begin(),preroll.end()); preroll.clear(); }
            phrase.insert(phrase.end(), input, input+count);
            if (speech) { quiet = 0; voiced += count; } else quiet += count;
            if (quiet >= rate * .65 || phrase.size() >= rate * 12ull) finishPhrase();
        } else {
            preroll.insert(preroll.end(),input,input+count);
            while (preroll.size() > rate / 5) preroll.pop_front();
        }
        float peak = 0;
        for (size_t i=0;i<count && !output.empty();++i) {
            out[i] = std::clamp(output.front() * (controls.gain / 100.f), -.97f, .97f);
            output.pop_front(); peak = std::max(peak,std::abs(out[i]));
        }
        controls.level = peak;
    }
};
