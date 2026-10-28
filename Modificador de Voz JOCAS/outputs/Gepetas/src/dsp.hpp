// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <rubberband/RubberBandLiveShifter.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <vector>

struct Controls {
    std::atomic<int> pitch{0}, effect{0}, gain{100}, gate{-60}, listenVolume{50};
    std::atomic<int> effectAmount{40}, robotHz{65}, radioDrive{25}, bass{0}, treble{0};
    std::atomic<bool> listen{false}, mute{false}, preserve{true};
    std::atomic<float> level{0};
};

class Processor {
    RubberBand::RubberBandLiveShifter shifter;
    const double sampleRate;
    const float hpCoefficient, lpCoefficient, bassCoefficient, trebleCoefficient;
    const float gateAttack, gateRelease, limiterRelease;
    std::vector<float> gated;
    double phase = 0;
    float gateEnvelope = 0, limiterGain = 1, hp = 0, last = 0, lp = 0, bassLow = 0, trebleLow = 0;
    size_t gateHold = 0;
    int previousPitch = 999, previousEffect = -1;
public:
    const size_t block = shifter.getBlockSize();
    explicit Processor(unsigned rate = 48000)
        : shifter(rate, 1, 0), sampleRate(rate),
          hpCoefficient(float(std::pow(.9622, 48000.0 / rate))),
          lpCoefficient(float(1.0 - std::pow(1.0 - .282, 48000.0 / rate))),
          bassCoefficient(float(1.0 - std::exp(-2.0 * 3.141592653589793 * 250 / rate))),
          trebleCoefficient(float(1.0 - std::exp(-2.0 * 3.141592653589793 * 3500 / rate))),
          gateAttack(float(1.0 - std::exp(-1.0 / (.002 * rate)))),
          gateRelease(float(1.0 - std::exp(-1.0 / (.100 * rate)))),
          limiterRelease(float(1.0 - std::exp(-1.0 / (.150 * rate)))),
          gated(shifter.getBlockSize()) {}
    void process(const float *input, float *output, Controls &c) {
        const int pitch = std::clamp(c.pitch.load(), -12, 12), effect = c.effect.load();
        if ((pitch == 0) != (previousPitch == 0)) shifter.reset();
        previousPitch = pitch;
        if (effect != previousEffect) { hp = last = lp = 0; previousEffect = effect; }
        const int gateDb = c.gate.load();
        const float threshold = gateDb <= -60 ? 0.f : std::pow(10.f, gateDb / 20.f);
        float peak = 0;
        for (size_t i = 0; i < block; ++i) peak = std::max(peak, std::abs(input[i]));
        // Hysteresis + 180 ms hold preserve quiet consonants between voiced sounds.
        if (threshold == 0) { gateEnvelope = 1; gateHold = 0; }
        else if (peak >= threshold * (gateHold ? .5f : 1.f)) gateHold = size_t(.180 * sampleRate);
        for (size_t i = 0; i < block; ++i) {
            const float target = threshold == 0 || gateHold ? 1.f : 0.f;
            if (gateHold) --gateHold;
            gateEnvelope += (target - gateEnvelope) * (target > gateEnvelope ? gateAttack : gateRelease);
            gated[i] = input[i] * gateEnvelope;
        }
        if (pitch == 0) std::copy(gated.begin(), gated.end(), output);
        else {
            shifter.setPitchScale(std::pow(2., pitch / 12.));
            shifter.setFormantOption(c.preserve ? RubberBand::RubberBandLiveShifter::OptionFormantPreserved : 0);
            const float *in[] = {gated.data()}; float *out[] = {output}; shifter.shift(in, out);
        }
        const float gain = std::clamp(c.gain.load(), 0, 200) / 100.f;
        const float amount = std::clamp(c.effectAmount.load(), 0, 100) / 100.f;
        const float drive = 1.f + std::clamp(c.radioDrive.load(), 0, 100) * .07f;
        const float bassGain = std::pow(10.f, std::clamp(c.bass.load(), -12, 12) / 20.f);
        const float trebleGain = std::pow(10.f, std::clamp(c.treble.load(), -12, 12) / 20.f);
        const double phaseStep = 2 * 3.14159265358979323846 * std::clamp(c.robotHz.load(), 20, 160) / sampleRate;
        float blockPeak = 0;
        for (size_t i = 0; i < block; ++i) {
            // Both effect branches use the already pitch-shifted signal. Never
            // blend input/gated (the original voice) back into a changed pitch.
            float x = output[i], wet = x;
            if (effect == 1) {
                wet = x * float(.35 + .65 * std::sin(phase));
                phase += phaseStep; if (phase >= 2 * 3.14159265358979323846) phase -= 2 * 3.14159265358979323846;
            } else if (effect == 2) {
                hp = hpCoefficient * (hp + x - last); last = x;
                lp += lpCoefficient * (hp - lp);
                wet = std::tanh(lp * drive) / drive;
            }
            x += amount * (wet - x);
            bassLow += bassCoefficient * (x - bassLow);
            trebleLow += trebleCoefficient * (x - trebleLow);
            x += (bassGain - 1.f) * bassLow + (trebleGain - 1.f) * (x - trebleLow);
            output[i] = x * gain;
            blockPeak = std::max(blockPeak, std::abs(output[i]));
        }
        // Reduce the complete waveform before overload; do not flatten speech peaks.
        const float ceiling = .95f;
        const float targetGain = blockPeak > ceiling ? ceiling / blockPeak : 1.f;
        limiterGain = std::min(limiterGain, targetGain);
        float outputPeak = 0;
        const bool muted = c.mute;
        for (size_t i = 0; i < block; ++i) {
            limiterGain += (targetGain - limiterGain) * limiterRelease;
            output[i] = muted ? 0.f : output[i] * limiterGain;
            outputPeak = std::max(outputPeak, std::abs(output[i]));
        }
        c.level = outputPeak;
    }
};
