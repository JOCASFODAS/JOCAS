// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#ifndef Q_OS_WIN
#include "platform_linux.hpp"
#include <QThread>

class GameMode {
    QString previous;
    bool enabled = false;
    QString target;
public:
    explicit GameMode(QString source = VirtualMic::source) : target(std::move(source)) {}
    void enable() {
        if (enabled) return;
        previous = QString::fromUtf8(pactl({"get-default-source"}));
        // If already selected by the user, stopping must keep their selection.
        if (previous == target) { previous.clear(); enabled = true; return; }
        pactl({"set-default-source", target});
        enabled = true;
        for (int attempt = 0; attempt < 20; ++attempt) {
            if (QString::fromUtf8(pactl({"get-default-source"})) == target) return;
            QThread::msleep(10);
        }
        throw std::runtime_error("Não consegui selecionar JOCAS Microfone como entrada padrão.");
    }
    void disable() {
        if (!enabled) return;
        if (!previous.isEmpty() && QString::fromUtf8(pactl({"get-default-source"})) == target) {
            bool available = false;
            for (auto source : QJsonDocument::fromJson(pactl({"-f", "json", "list", "sources"})).array())
                if (source.toObject()["name"].toString() == previous) { available = true; break; }
            // Don't overwrite a user's newer choice, or select an unplugged device.
            if (available) pactl({"set-default-source", previous});
        }
        enabled = false; previous.clear();
    }
    ~GameMode() { try { disable(); } catch (...) {} }
};
#endif
