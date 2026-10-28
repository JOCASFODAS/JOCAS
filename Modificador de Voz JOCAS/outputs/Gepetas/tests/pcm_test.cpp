#include "../src/pcm.hpp"
#include <iostream>
#include <limits>
int main() {
    auto near = [](float got, float expected) { if (std::abs(got-expected) > .0001f) throw std::runtime_error("PCM conversion failed"); };
    const unsigned char negative16[]{0,128}, positive16[]{255,127};
    const unsigned char negative24[]{0,0,128}, positive24[]{255,255,127};
    const unsigned char negative32[]{0,0,0,128}, positive32[]{255,255,255,127};
    near(pcmSample(negative16,16,false),-1); near(pcmSample(positive16,16,false),1);
    near(pcmSample(negative24,24,false),-1); near(pcmSample(positive24,24,false),1);
    near(pcmSample(negative32,32,false),-1); near(pcmSample(positive32,32,false),1);
    float value=.375f; near(pcmSample(reinterpret_cast<unsigned char *>(&value),32,true),.375f);
    value=std::numeric_limits<float>::quiet_NaN(); near(pcmSample(reinterpret_cast<unsigned char *>(&value),32,true),0);
    bool rejected=false; try { pcmSample(negative16,8,false); } catch (const std::exception &) { rejected=true; }
    if (!rejected) return 1;
    std::cout << "PCM16/24/32, float32 and invalid format: OK\n";
}
