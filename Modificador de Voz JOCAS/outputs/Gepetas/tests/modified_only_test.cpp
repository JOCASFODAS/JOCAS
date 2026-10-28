#include "../src/dsp.hpp"
#include "../src/voice_profile.hpp"
#include <complex>
#include <iostream>
#include <stdexcept>
static void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
static double amplitude(const std::vector<float> &signal, double frequency, unsigned rate) {
    std::complex<double> sum{};
    for (size_t i=0;i<signal.size();++i) {
        const double window=.5-.5*std::cos(2*M_PI*i/(signal.size()-1));
        sum += double(signal[i])*window*std::polar(1.,-2*M_PI*frequency*i/rate);
    }
    return std::abs(sum)*4/signal.size();
}
int main() {
    for (unsigned rate:{44100u,48000u,96000u}) for(int effect:{0,1,2}) for(int amount:{0,30,100}) {
        auto profile=voicePreset(1); Controls c;
        c.pitch=profile.pitch;c.preserve=profile.preserve;c.bass=profile.bass;c.treble=profile.treble;
        c.effect=effect;c.effectAmount=amount;
        Processor dsp(rate);std::vector<float> in(dsp.block),out(dsp.block),tail;
        size_t t=0;
        for(size_t b=0;b<size_t(4*rate/dsp.block);++b) {
            for(auto &v:in) v=.1*std::sin(2*M_PI*440*t++/rate);
            dsp.process(in.data(),out.data(),c);
            if(b>size_t(2*rate/dsp.block))tail.insert(tail.end(),out.begin(),out.end());
        }
        const double shifted=amplitude(tail,440*std::pow(2.,-5./12),rate);
        const double original=amplitude(tail,440,rate);
        check(shifted>.005,"Modified voice missing");
        check(original/shifted<.01,"Original tone mixed into modified output");
        std::cout<<rate<<" Hz effect="<<effect<<" amount="<<amount<<" original/modified="<<20*std::log10(std::max(1e-12,original/shifted))<<" dB OK\n";
        c.mute=true;dsp.process(in.data(),out.data(),c);
        check(std::all_of(out.begin(),out.end(),[](float v){return v==0;}),"Mute leaked audio");
    }
}
