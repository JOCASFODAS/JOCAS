#include "../src/ai_speech.hpp"
#include <QThread>
#include <QElapsedTimer>
#include <iostream>
#include <stdexcept>
static void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
int main(int argc,char**argv){
 QCoreApplication app(argc,argv);check(argc==3,"python and fixture args required");
 Controls controls;AiSettings settings;settings.enabled=true;settings.program=argv[1];settings.arguments={argv[2]};
 std::vector<float> in(512),out(512);bool ready=false,busy=false;double sent=0;
 AiSpeech bridge(settings,48000,[&](QString s){ready|=s=="fixture ready";busy|=s.contains("processando");});
 auto block=[&](bool speech){for(size_t i=0;i<in.size();++i)in[i]=speech?.18f*std::sin(2*3.141592653589793*440*(sent+i)/48000):0;sent+=in.size();bridge.processBlock(in.data(),out.data(),out.size(),controls);};
 for(int i=0;i<400&&!ready;++i){block(false);QThread::msleep(2);}check(ready,"Child did not become ready");
 // Until synthesis returns, output must be silence even while raw input is loud.
 for(int i=0;i<45;++i){block(true);for(float f:out)check(f==0,"Original microphone leaked");}
 for(int i=0;i<65;++i) { block(false); }
 check(busy,"VAD failed to dispatch phrase");
 controls.mute=true;block(false);controls.mute=false;block(false);
 for(int i=0;i<250;++i){block(false);for(float f:out)check(f==0,"Late result played after mute/unmute");QThread::msleep(2);}
 busy=false;for(int i=0;i<45;++i)block(true);for(int i=0;i<65;++i)block(false);check(busy,"New phrase not dispatched after mute");
 double energy=0;size_t samples=0;int crossings=0;float previous=0;
 for(int i=0;i<350;++i){block(false);for(float f:out){energy+=f*f;if(previous<0&&f>=0)++crossings;previous=f;if(std::abs(f)>.0001)++samples;}QThread::msleep(2);}
 check(energy>100&&samples>15000,"Synthesized speech was not rendered");check(crossings>420&&crossings<460,"Output is not synthesized 880Hz fixture");
 controls.mute=true;block(false);for(float f:out)check(f==0,"Mute is not silent");
 AiSettings missing=settings;missing.program="/nonexistent/jocas-python";bool failed=false;
 AiSpeech dead(missing,48000,[&](QString s){failed|=s.contains("parou");});
 for(int i=0;i<50;++i){dead.processBlock(in.data(),out.data(),out.size(),controls);QThread::msleep(2);}check(failed,"Missing child did not report failure");
 std::cout<<"PASS native AI: no dry voice, VAD, synthesis/resampling, stale result discarded after mute, resume and silent child failure\n";
}
