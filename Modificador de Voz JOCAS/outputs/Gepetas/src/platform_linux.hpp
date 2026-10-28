// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "devices.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <stdexcept>
static QByteArray pactl(const QStringList &args) {
    QProcess p;
    p.start("pactl", args);
    if (!p.waitForStarted(2000)) throw std::runtime_error("O comando pactl não está instalado (pacote libpulse).");
    if (!p.waitForFinished(4000)) {
        p.kill(); p.waitForFinished();
        throw std::runtime_error("O servidor de áudio demorou para responder.");
    }
    if (p.exitCode() != 0) throw std::runtime_error(p.readAllStandardError().trimmed().toStdString());
    return p.readAllStandardOutput().trimmed();
}

#ifndef JOCAS_VIRTUAL_NAMESPACE
#define JOCAS_VIRTUAL_NAMESPACE "gepetas"
#endif
class VirtualMic {
    QStringList modules;
    void load(QStringList args) {
        auto id = QString::fromUtf8(pactl(args)); bool ok = false; id.toUInt(&ok);
        if (!ok) throw std::runtime_error("O servidor não retornou o identificador do microfone virtual.");
        modules.prepend(id);
    }
public:
    static constexpr const char *bus = JOCAS_VIRTUAL_NAMESPACE "_bus";
    static constexpr const char *source = JOCAS_VIRTUAL_NAMESPACE "_mic";
    QString cleanupError;
    QString target() const { return QString::fromLatin1(bus); }
    void prepareLevel() {
        // These are our private virtual devices, not the physical microphone.
        // A restored 27% source volume attenuates speech by about 34 dB.
        pactl({"set-sink-volume", bus, "100%"});
        pactl({"set-sink-mute", bus, "0"});
        pactl({"set-source-volume", source, "100%"});
        pactl({"set-source-mute", source, "0"});
    }
    void create() {
        // The single-instance lock is held. Reclaim only modules bearing our ownership marker.
        const auto existing = QJsonDocument::fromJson(pactl({"-f", "json", "list", "modules"})).array();
        QStringList oldSources, oldSinks;
        for (auto entry : existing) {
            auto o = entry.toObject(); const QString args = o["argument"].toString();
            if (!args.contains("gepetas.owner=" JOCAS_VIRTUAL_NAMESPACE "-voz-v1")) continue;
            const auto id = QString::number(o["index"].toInt());
            if (o["name"] == "module-remap-source" && args.contains("source_name=" JOCAS_VIRTUAL_NAMESPACE "_mic")) oldSources << id;
            if (o["name"] == "module-null-sink" && args.contains("sink_name=" JOCAS_VIRTUAL_NAMESPACE "_bus")) oldSinks << id;
        }
        for (auto id : oldSources + oldSinks) pactl({"unload-module", id});
        try {
            load({"load-module", "module-null-sink", "sink_name=" JOCAS_VIRTUAL_NAMESPACE "_bus", "rate=48000", "channels=1", "channel_map=mono",
                  "sink_properties=\"device.description='JOCAS Processamento' gepetas.owner=" JOCAS_VIRTUAL_NAMESPACE "-voz-v1 device.class=abstract priority.session=1\""});
            load({"load-module", "module-remap-source", "master=" JOCAS_VIRTUAL_NAMESPACE "_bus.monitor", "source_name=" JOCAS_VIRTUAL_NAMESPACE "_mic",
                  "channels=1", "channel_map=mono", "master_channel_map=mono", "remix=no",
                  "source_properties=\"device.description='JOCAS Microfone' gepetas.owner=" JOCAS_VIRTUAL_NAMESPACE "-voz-v1 device.class=virtual priority.session=1\""});
            prepareLevel();
        } catch (...) { destroy(); throw; }
    }
    void destroy() {
        cleanupError.clear();
        QStringList failed;
        for (const auto &id : modules) {
            try { pactl({"unload-module", id}); }
            catch (const std::exception &e) { failed << id; cleanupError = QString::fromUtf8(e.what()); }
        }
        modules = failed;
    }
    ~VirtualMic() { if (!modules.isEmpty()) destroy(); }
};

inline AudioInventory enumerateAudioDevices() {
    AudioInventory result;
    const auto sources = QJsonDocument::fromJson(pactl({"-f", "json", "list", "sources"})).array();
    const auto sinks = QJsonDocument::fromJson(pactl({"-f", "json", "list", "sinks"})).array();
    for (auto value : sources) {
        auto o = value.toObject(); const auto name = o["name"].toString();
        const auto monitorOf = o["monitor_of_sink"];
        const bool isMonitor = monitorOf.isString() ? monitorOf.toString() != "n/a"
            : monitorOf.isDouble() && monitorOf.toDouble() != 4294967295.0 && monitorOf.toDouble() != -1;
        if (name.startsWith(JOCAS_VIRTUAL_NAMESPACE "_") || name.endsWith(".monitor") || isMonitor ||
            o["properties"].toObject()["device.class"].toString() == "monitor") continue;
        result.inputs.append({name, o["description"].toString(name)});
    }
    for (auto value : sinks) {
        auto o = value.toObject(); const auto name = o["name"].toString();
        if (!name.startsWith(JOCAS_VIRTUAL_NAMESPACE "_")) result.outputs.append({name, o["description"].toString(name)});
    }
    result.defaultInput = QString::fromUtf8(pactl({"get-default-source"}));
    result.defaultOutput = QString::fromUtf8(pactl({"get-default-sink"}));
    result.virtualOutput = VirtualMic::bus;
    return result;
}
inline QString platformLabel() { return "LINUX  /  PIPEWIRE"; }
inline QString platformAudioError() { return "Não consegui acessar o áudio. Verifique o PipeWire e clique em Atualizar."; }
inline QString discordSourceName() { return "JOCAS Microfone"; }
inline QString platformStoppedMessage() { return "Parado. O microfone virtual foi desconectado."; }
