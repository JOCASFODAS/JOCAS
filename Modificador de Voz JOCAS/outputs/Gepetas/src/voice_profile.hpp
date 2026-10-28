// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
struct VoiceProfile {
    int pitch = 0, effect = 0, gain = 100, gate = -60;
    bool preserve = true;
    int amount = 40, robotHz = 65, radioDrive = 25, bass = 0, treble = 0;
};
inline VoiceProfile voicePreset(int id) {
    VoiceProfile p;
    switch (id) {
        case 1: p.pitch = -5; p.preserve = false; p.bass = 1; p.treble = 2; break;
        case 2: p.pitch = 3; break;
        case 3: p.pitch = -2; p.effect = 1; p.amount = 40; break;
        case 4: p.pitch = -2; p.effect = 2; p.amount = 65; break;
        case 5: p.pitch = -5; p.effect = 1; p.amount = 30; break;
    }
    return p;
}
