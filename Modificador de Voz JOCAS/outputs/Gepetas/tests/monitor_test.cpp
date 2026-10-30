// Compile with -I pointing at src/ (or an older snapshot for regression comparison).
#define JOCAS_AUDIO_CLIENT_NAME "JOCAS Synthetic Monitor Test"
#include "audio.hpp"
#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include <pthread.h>

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (argc != 4) return 2;
    Controls controls; controls.pitch = -9; controls.listen = true; controls.listenVolume = 100;
    AudioWorker worker(controls, argv[1], argv[2], argv[3]);
    worker.onReady = [] {
        int policy = 0; sched_param priority{};
        pthread_getschedparam(pthread_self(), &policy, &priority);
        std::cerr << "READY scheduler=" << policy << " priority=" << priority.sched_priority << '\n';
    };
    QObject::connect(&worker, &QThread::finished, &app, &QCoreApplication::quit);
    QTimer::singleShot(15000, [&] { worker.requestInterruption(); });
    worker.start(); app.exec(); worker.wait();
    if (!worker.error.isEmpty()) { std::cerr << worker.error.toStdString() << '\n'; return 1; }
    return 0;
}
