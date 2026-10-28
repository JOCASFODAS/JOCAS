// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <stdexcept>

// WASAPI capture uses the native mix format; this avoids changing the microphone's
// format or depending on capture-side automatic resampling on Windows 11.
inline float pcmSample(const unsigned char *p, unsigned bits, bool floating) {
    if (floating && bits == 32) {
        float value; std::memcpy(&value, p, sizeof(value));
        return std::isfinite(value) ? value : 0.f;
    }
    if (!floating && bits == 16) {
        int16_t value; std::memcpy(&value, p, sizeof(value)); return value / 32768.f;
    }
    if (!floating && bits == 24) {
        int32_t value = p[0] | (int32_t(p[1]) << 8) | (int32_t(p[2]) << 16);
        if (value & 0x800000) value -= 0x1000000;
        return value / 8388608.f;
    }
    if (!floating && bits == 32) {
        int32_t value; std::memcpy(&value, p, sizeof(value)); return float(value / 2147483648.0);
    }
    throw std::runtime_error("Formato do microfone não suportado. Selecione PCM de 16/24/32 bits ou float de 32 bits no Windows.");
}
