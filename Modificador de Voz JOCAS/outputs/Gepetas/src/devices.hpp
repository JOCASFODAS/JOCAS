// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QString>
#include <QList>

struct AudioDevice { QString id, label; };
struct AudioInventory {
    QList<AudioDevice> inputs, outputs;
    QString defaultInput, defaultOutput, virtualOutput, message;
    bool virtualReady = true;
};
