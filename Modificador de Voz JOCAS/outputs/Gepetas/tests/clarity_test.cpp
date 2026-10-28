#include "../src/dsp.hpp"
#include "../src/voice_profile.hpp"
#include <iostream>
#include <stdexcept>
static void check(bool pass, const char *message) { if (!pass) throw std::runtime_error(message); }
int main() {
    for (unsigned rate : {44100u, 48000u, 96000u}) {
        Controls c; Processor dsp(rate);
        std::vector<float> in(dsp.block), out(dsp.block);
        size_t t = 0;
        // Quiet speech passes unchanged when noise cutting is disabled.
        for (auto &v : in) v = .003f * std::sin(2 * 3.141592653589793 * 750 * t++ / rate);
        dsp.process(in.data(), out.data(), c);
        check(in == out, "Quiet syllable changed in Natural with gate disabled");
        // A consonant at -40 dB between louder syllables survives even a -20 dB gate.
        c.gate = -20;
        for (int b = 0; b < 30; ++b) {
            for (auto &v : in) v = .2f * std::sin(2 * 3.141592653589793 * 750 * t++ / rate);
            dsp.process(in.data(), out.data(), c);
        }
        double before = 0, after = 0;
        for (unsigned b = 0; b < unsigned(.120 * rate / dsp.block); ++b) {
            for (auto &v : in) v = .01f * std::sin(2 * 3.141592653589793 * 750 * t++ / rate);
            dsp.process(in.data(), out.data(), c);
            for (size_t i = 0; i < dsp.block; ++i) { before += in[i]*in[i]; after += out[i]*out[i]; }
        }
        check(after/before > .98, "Noise gate removed a quiet consonant");
        // An actual quiet interval still closes the gate.
        for (int b = 0; b < int(rate * 2 / dsp.block); ++b) dsp.process(in.data(), out.data(), c);
        check(c.level < .000001, "Gate failed to close after silence");
        for (int id = 0; id < 6; ++id) {
            auto p = voicePreset(id);
            check(p.gate == -60 && p.gain == 100, "Unsafe preset defaults");
        }
        std::cout << rate << " Hz: quiet speech + consonant hold + gate close OK\n";
    }
    Controls c; c.gain = 200; Processor dsp;
    std::vector<float> in(dsp.block), out(dsp.block);
    for (size_t i=0; i<in.size(); ++i) in[i]=.8f*std::sin(2*3.141592653589793*750*i/48000);
    for (int b=0; b<50; ++b) dsp.process(in.data(),out.data(),c);
    for (size_t i=0; i<in.size(); ++i)
        check(std::abs(out[i] - in[i]*(.95f/.8f)) < .00001, "Limiter flattened waveform");
    Controls robot; robot.effect=1; robot.effectAmount=40; Processor robotic;
    for (int b=0; b<30; ++b) {
        robotic.process(in.data(),out.data(),robot);
        for (size_t i=0; i<in.size(); ++i) if (std::abs(in[i]) > .001)
            check(out[i]/in[i] >= .479, "Default robotic effect erased speech carrier");
    }
    std::cout << "Limiter preserves waveform; robot retains speech carrier OK\n";
}
