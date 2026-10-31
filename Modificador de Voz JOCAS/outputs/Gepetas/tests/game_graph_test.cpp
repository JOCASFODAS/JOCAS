// Synthetic-only full virtual microphone fixture; no hardware microphone.
#define JOCAS_AUDIO_CLIENT_NAME "JOCAS Graph Synthetic Test"
#define JOCAS_VIRTUAL_NAMESPACE "jocas_graph_test"
#include "../src/audio.hpp"
#include "../src/platform.hpp"
#include <QCoreApplication>
#include <QTimer>
#include <iostream>
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    if(argc!=3) return 2;
    Controls controls; controls.pitch=-9; controls.listen=true; controls.listenVolume=100;
    VirtualMic mic; mic.create();
    AudioWorker worker(controls,argv[1],argv[2],mic.target());
    worker.onReady=[] {std::cerr<<"READY\n";};
    QObject::connect(&worker,&QThread::finished,&app,&QCoreApplication::quit);
    QTimer::singleShot(40000,[&]{worker.requestInterruption();});
    worker.start();app.exec();worker.wait();
    if(!worker.error.isEmpty()){std::cerr<<worker.error.toStdString()<<'\n';return 1;}
    return 0;
}
