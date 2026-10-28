// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#ifndef Q_OS_WIN
#include "platform_linux.hpp"
#include <QSet>
#include <QThread>
#include <functional>

// Change only Discord's live capture streams, never the JOCAS input or other apps.
class DiscordRoute {
    QSet<int> attempted;
    QString target;
    QStringList binaries;
    std::function<QByteArray(const QStringList &)> query;
public:
    explicit DiscordRoute(QString source = VirtualMic::source,
                          QStringList apps = {"discord", "discordcanary", "discordptb", "vesktop"},
                          std::function<QByteArray(const QStringList &)> run = pactl)
        : target(std::move(source)), binaries(std::move(apps)), query(std::move(run)) {}
    void reset() { attempted.clear(); }
    QString update(bool retry = false) {
        if (retry) reset();
        int targetIndex = -1;
        for (const auto &entry : QJsonDocument::fromJson(query({"-f", "json", "list", "sources"})).array()) {
            const auto source = entry.toObject();
            if (source["name"].toString() == target) targetIndex = source["index"].toInt();
        }
        if (targetIndex < 0) return "Inicie o microfone virtual para conectar o Discord.";
        bool found = false, connected = false, failed = false;
        QSet<int> live;
        for (const auto &entry : QJsonDocument::fromJson(query({"-f", "json", "list", "source-outputs"})).array()) {
            const auto stream = entry.toObject();
            const auto binary = stream["properties"].toObject()["application.process.binary"].toString().section('/', -1).toLower();
            if (!binaries.contains(binary)) continue;
            found = true;
            const int id = stream["index"].toInt(); live.insert(id);
            if (stream["source"].toInt() == targetIndex) { attempted.insert(id); connected = true; continue; }
            if (attempted.contains(id)) { failed = true; continue; }
            attempted.insert(id);
            try {
                query({"move-source-output", QString::number(id), target});
                bool verified = false;
                for (int attempt = 0; attempt < 25 && !verified; ++attempt) {
                    for (const auto &current : QJsonDocument::fromJson(query({"-f", "json", "list", "source-outputs"})).array()) {
                        const auto item = current.toObject();
                        if (item["index"].toInt() == id && item["source"].toInt() == targetIndex) verified = true;
                    }
                    if (!verified) QThread::msleep(10);
                }
                connected |= verified; failed |= !verified;
            } catch (...) { failed = true; }
        }
        attempted.intersect(live);
        if (failed) return "Selecione JOCAS Microfone em Voz e vídeo no Discord; depois clique em Conectar Discord.";
        if (connected) return "Discord conectado ao JOCAS Microfone: recebe somente a saída processada.";
        return found ? "Confira a entrada do Discord." : "Abra uma chamada ou o teste de microfone do Discord para conectar. No navegador, selecione JOCAS Microfone manualmente.";
    }
};
#endif
