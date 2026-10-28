#define JOCAS_AUDIO_CLIENT_NAME "JOCAS AI Synthetic Test"
#define JOCAS_VIRTUAL_NAMESPACE "jocas_ai_test"
#include "../src/audio.hpp"
#include "../src/platform.hpp"
#include <QCoreApplication>
#include <QTimer>
#include <iostream>
int main(int argc,char**argv){
 QCoreApplication app(argc,argv);if(argc!=6)return 2;
 Controls c;c.listen=true;c.listenVolume=50;c.gain=100;
 AiSettings ai;ai.enabled=true;ai.program=argv[3];ai.arguments={argv[4],"serve"};
 const QString root=argv[5];ai.config={{"mode","local"},{"recognizer",root+"/models/recognizer/base"},{"voicePath",root+"/models/voices/cadu/pt_BR-cadu-medium.onnx"}};
 VirtualMic mic;mic.create();
 AudioWorker worker(c,argv[1],argv[2],mic.target(),ai);
 worker.onReady=[&]{std::cerr<<"AUDIO READY\n";};
 worker.onAiStatus=[](QString s){std::cerr<<s.toStdString()<<std::endl;};
 QTimer::singleShot(500,[&]{mic.prepareLevel();});
 QTimer::singleShot(26000,[&]{worker.requestInterruption();});
 QObject::connect(&worker,&QThread::finished,&app,&QCoreApplication::quit);
 worker.start();app.exec();worker.wait();mic.destroy();
 if(!worker.error.isEmpty()){std::cerr<<worker.error.toStdString()<<std::endl;return 1;}
 std::cout<<"CLEANUP OK\n";
}
