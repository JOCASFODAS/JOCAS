#include "../src/dsp.hpp"
#include <iostream>
#include <stdexcept>

static void require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
int main() {
    for (unsigned rate : {44100u, 48000u, 96000u}) for (int pitch : {-12, -5, 0, 6, 12}) {
        Controls controls; controls.pitch = pitch; Processor dsp(rate);
        std::vector<float> in(dsp.block), out(dsp.block), tail;
        size_t t = 0;
        for (int b = 0; b < 240; ++b) {
            for (auto &v : in) v = .2f * std::sin(2 * 3.14159265358979323846 * 220 * t++ / rate);
            dsp.process(in.data(), out.data(), controls);
            if (b > 120) tail.insert(tail.end(), out.begin(), out.end());
            for (auto v : out) require(std::isfinite(v) && std::abs(v) <= .981, "Invalid output");
        }
        int crossings = 0;
        for (size_t i = 1; i < tail.size(); ++i) if (tail[i-1] <= 0 && tail[i] > 0) ++crossings;
        double freq = crossings * double(rate) / tail.size(), expected = 220 * std::pow(2., pitch / 12.);
        require(std::abs(freq - expected) < 4, "Pitch frequency mismatch");
        controls.mute = true; dsp.process(in.data(), out.data(), controls);
        require(std::all_of(out.begin(), out.end(), [](float v) { return v == 0; }), "Mute leaked audio");
        std::cout << rate << " Hz / tom " << pitch << ": " << freq << " Hz (esperado " << expected << ") OK\n";
    }
    for (int effect : {0, 1, 2}) {
        Controls c; c.effect = effect; c.gain = 200; Processor dsp;
        std::vector<float> in(dsp.block, 10.f), out(dsp.block);
        dsp.process(in.data(), out.data(), c);
        for (auto v : out) require(std::isfinite(v) && std::abs(v) <= .981, "Limiter failed");
        std::fill(in.begin(), in.end(), 0.f);
        for (int i = 0; i < 100; ++i) dsp.process(in.data(), out.data(), c);
        for (auto v : out) require(std::abs(v) < .0001, "Silence unstable");
    }
    Controls c; c.gate = -30; Processor dsp;
    std::vector<float> in(dsp.block, .0001f), out(dsp.block);
    for (int i = 0; i < 100; ++i) dsp.process(in.data(), out.data(), c);
    require(c.level < .00001, "Gate failed");
    std::cout << "Mudo, limitador, silêncio, efeitos e corte de ruído: OK\n";
}
