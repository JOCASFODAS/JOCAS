#include "../src/platform_windows.hpp"
#include "../src/audio_windows.hpp"
#include <QCoreApplication>
#include <QTimer>
#include <iostream>
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        const auto inventory = enumerateAudioDevices();
        if (app.arguments().contains("--list")) {
            for (const auto &d : inventory.inputs) std::cout << "IN " << d.label.toStdString() << '\n';
            for (const auto &d : inventory.outputs) std::cout << "OUT " << d.label.toStdString() << '\n';
            std::cout << "VIRTUAL_READY " << inventory.virtualReady << '\n'; return 0;
        }
        QString mic, phones;
        for (const auto &d : inventory.inputs) if (d.label.contains("JOCAS_Test_Microphone")) mic = d.id;
        for (const auto &d : inventory.outputs) if (d.label.contains("JOCAS_Test_Headphones")) phones = d.id;
        if (mic.isEmpty() || phones.isEmpty() || !inventory.virtualReady)
            throw std::runtime_error("Synthetic devices missing; refusing to open hardware devices.");
        Controls c; c.pitch = 12;
        AudioWorker worker(c, mic, phones, inventory.virtualOutput);
        worker.onReady = [] { std::cerr << "READY\n"; };
        QObject::connect(&worker, &QThread::finished, &app, &QCoreApplication::quit);
        QTimer::singleShot(3000, [&] { c.listen = true; std::cerr << "LISTEN ON\n"; });
        QTimer::singleShot(6000, [&] { c.listen = false; std::cerr << "LISTEN OFF\n"; });
        QTimer::singleShot(9000, [&] { c.mute = true; std::cerr << "MUTE\n"; });
        QTimer::singleShot(12000, [&] { worker.requestInterruption(); });
        worker.start(); app.exec(); worker.wait();
        if (!worker.error.isEmpty()) throw std::runtime_error(worker.error.toStdString());
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
