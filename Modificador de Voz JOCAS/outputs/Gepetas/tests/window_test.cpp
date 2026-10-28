#define JOCAS_AUDIO_CLIENT_NAME "JOCAS Synthetic Window Test"
#define JOCAS_VIRTUAL_NAMESPACE "jocas_integration"
#define main gepetas_application_main
#include "../src/main.cpp"
#undef main
#include <iostream>

// Runs the real window and virtual-device lifecycle using synthetic fixtures only.
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setApplicationName("GepetasTest"); app.setOrganizationName("GepetasTest");
    QLockFile lock(lockPath());
    if (!lock.tryLock(0)) return 3;
    if (argc != 3) return 2;
    Window window; window.show();
    bool started = false;
    auto fail = [&](const char *message) { std::cerr << message << '\n'; app.exit(1); };
    auto button = [&](const char *text) -> QPushButton * {
        for (auto *b : window.findChildren<QPushButton *>()) if (b->text().contains(text)) return b;
        return nullptr;
    };
    QTimer::singleShot(300, [&] {
        for (auto *box : window.findChildren<QComboBox *>()) {
            const QString name = box->accessibleName();
            if (name != "Microfone de entrada" && name != "Fones para escuta") continue;
            const QString target = name == "Microfone de entrada" ? argv[1] : argv[2];
            const int i = box->findData(target);
            if (i < 0) { fail("Synthetic fixture missing"); return; }
            box->setCurrentIndex(i);
        }
        auto *b = button("Iniciar microfone"); if (!b || !b->isEnabled()) { fail("Start disabled"); return; }
        button("Grave")->click();
        for (auto *s : window.findChildren<QSlider *>()) if (s->accessibleName() == "Tom da voz") s->setValue(-5);
        // A single sine has no speech formants; disable formant preservation for
        // this frequency/amplitude fixture (speech settings are tested separately).
        for (auto *check : window.findChildren<QCheckBox *>())
            if (check->text().startsWith("Preservar o timbre")) check->setChecked(false);
        // This GUI test must never redirect a real game to synthetic test audio.
        if (auto *compatibility = window.findChild<QCheckBox *>("gameMode")) compatibility->setChecked(false);
        if (auto *routing = window.findChild<QCheckBox *>("discordAuto")) routing->setChecked(false);
        b->click(); started = true;
        QSettings current;
        std::cerr << "FIXTURE preserve=" << current.value("preserve").toBool()
                  << " pitch=" << current.value("pitch").toInt() << "\n";
        if (current.value("preserve").toBool()) fail("Synthetic fixture failed to disable formants");
    });
    QTimer::singleShot(2500, [&] {
        bool live = false;
        for (auto *l : window.findChildren<QLabel *>()) if (l->text().contains("Ao vivo")) live = true;
        if (!live || !started) { fail("Window failed to reach live state"); return; }
        std::cerr << "READY\n";
    });
    auto listen = window.findChild<QCheckBox *>("listen");
    QTimer::singleShot(3500, [&] { listen->setChecked(true); std::cerr << "LISTEN ON\n"; });
    QTimer::singleShot(6500, [&] { listen->setChecked(false); std::cerr << "LISTEN OFF\n"; });
    QTimer::singleShot(9500, [&] { button("Silenciar")->click(); std::cerr << "MUTE\n"; });
    QTimer::singleShot(12000, [&] { button("Parar microfone")->click(); });
    // Wait for the asynchronous worker stop and server-side removal, rather
    // than assuming coarse GUI timers guarantee a full second between callbacks.
    auto *cleanup = new QTimer(&window); cleanup->setInterval(100);
    QObject::connect(cleanup, &QTimer::timeout, &window, [&] {
        if (pactl({"-f", "json", "list", "sources"}).contains("jocas_integration_mic")) return;
        cleanup->stop(); std::cerr << "CLEANUP OK\n"; app.quit();
    });
    QTimer::singleShot(13000, cleanup, [=] { cleanup->start(); });
    QTimer::singleShot(21000, [&] { fail("Virtual microphone not cleaned up within timeout"); });
    return app.exec();
}
