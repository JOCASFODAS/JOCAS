#define JOCAS_AUDIO_CLIENT_NAME "JOCAS Synthetic Engine Test"
#include "../src/audio.hpp"
#include <QCoreApplication>
#include <QTimer>
#include <iostream>
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (argc != 4) return 2;
    Controls c; c.pitch = 12;
    AudioWorker worker(c, argv[1], argv[2], argv[3]);
    worker.onReady = [] { std::cerr << "READY\n"; };
    QObject::connect(&worker, &QThread::finished, &app, &QCoreApplication::quit);
    QTimer::singleShot(3000, [&] { c.listen = true; std::cerr << "LISTEN ON\n"; });
    QTimer::singleShot(6000, [&] { c.listen = false; std::cerr << "LISTEN OFF\n"; });
    QTimer::singleShot(9000, [&] { c.mute = true; std::cerr << "MUTE\n"; });
    QTimer::singleShot(12000, [&] { worker.requestInterruption(); });
    worker.start(); app.exec(); worker.wait();
    if (!worker.error.isEmpty()) { std::cerr << worker.error.toStdString() << '\n'; return 1; }
    return 0;
}
