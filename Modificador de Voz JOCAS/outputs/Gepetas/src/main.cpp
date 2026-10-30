// SPDX-License-Identifier: GPL-2.0-or-later
#include "audio.hpp"
#include "voice_profile.hpp"
#include "ai_panel.hpp"
#include <QtWidgets>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QLockFile>
#include <QStandardPaths>
#include <csignal>

static volatile std::sig_atomic_t interrupted = 0;
static void handleSignal(int) { interrupted = 1; }
static QString lockPath() {
    const auto directory = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/Gepetas";
    QDir().mkpath(directory);
    return directory + "/gepetas-voz.lock";
}

#include "platform.hpp"
#include "game_mode.hpp"
#include "discord_route.hpp"

class Window : public QWidget {
    Controls controls;
#ifndef Q_OS_WIN
    GameMode gameMode;
    QCheckBox *games;
    QLabel *gameHint;
    bool audioReady = false;
    DiscordRoute discordRoute;
    QCheckBox *discordAuto;
    QLabel *discordHint;
    void updateDiscord(bool retry = false) {
        if (preview || !audioReady || !discordAuto->isChecked()) return;
        try { discordHint->setText(discordRoute.update(retry)); }
        catch (...) { discordHint->setText("Não consegui verificar a entrada. Selecione JOCAS Microfone no Discord."); }
    }
    void updateGameMode() {
        try {
            if (audioReady && games->isChecked() && !preview) {
                gameMode.enable();
                gameHint->setText("JOCAS é a entrada padrão. Abra o jogo agora. Se já estava aberto, troque o microfone nele ou reabra o jogo.");
            } else {
                gameMode.disable();
                gameHint->setText(games->isChecked() ? "Inicie o microfone virtual antes de abrir o jogo."
                                                  : "No jogo, selecione JOCAS Microfone como entrada de voz.");
            }
        } catch (const std::exception &e) { gameHint->setText(QString::fromUtf8(e.what())); }
    }
#endif
    VirtualMic virtualMic;
    AudioWorker *worker = nullptr;
    bool changingVoice = false, holdVirtualMic = false, previousMute = false;
    QComboBox *microphone, *headphones;
    QPushButton *start, *refresh, *mute;
    QCheckBox *listen, *preserve;
    QSlider *pitch, *gain, *gate, *listenVolume;
    QSlider *amount, *robotHz, *radioDrive, *bass, *treble;
    QComboBox *effectType;
    QWidget *customPanel;
    QLabel *gateHint = nullptr;
    VoiceProfile customVoice;
    bool updatingVoice = false;
    int selectedVoice = 0;
    QLabel *status, *statusDot, *monitorHint;
    QProgressBar *meter;
    QButtonGroup *presets;
    bool preview;
    QSettings settings;
    AiPanel *aiPanel = nullptr;
    QString lastInput, lastOutput;

    QLabel *label(const QString &s, const char *name = "caption") {
        auto *l = new QLabel(s); l->setObjectName(name); l->setWordWrap(true); return l;
    }
    QSlider *slider(QVBoxLayout *layout, const QString &title, int min, int max, int value,
                    std::function<QString(int)> format, std::function<void(int)> update) {
        auto *row = new QHBoxLayout;
        auto *readout = label(format(value), "value"); readout->setAlignment(Qt::AlignRight);
        row->addWidget(label(title, "field")); row->addWidget(readout);
        auto *s = new QSlider(Qt::Horizontal); s->setRange(min, max); s->setValue(value);
        s->setAccessibleName(title); s->setMinimumHeight(24);
        layout->addLayout(row); layout->addWidget(s);
        connect(s, &QSlider::valueChanged, this, [readout, format, update](int v) { readout->setText(format(v)); update(v); });
        return s;
    }
    void setStatus(QString text, bool live = false, bool error = false) {
        status->setText(text);
        statusDot->setStyleSheet(QString("color:%1;font-size:20px;").arg(error ? "#ff8c91" : live ? "#80ecc4" : "#8994ab"));
    }
    VoiceProfile currentVoice() const {
        return {controls.pitch.load(), controls.effect.load(), controls.gain.load(), controls.gate.load(),
                controls.preserve.load(), controls.effectAmount.load(), controls.robotHz.load(),
                controls.radioDrive.load(), controls.bass.load(), controls.treble.load()};
    }
    VoiceProfile readVoice(const QString &prefix) {
        auto number = [&](const char *key, int fallback, int low, int high) {
            return std::clamp(settings.value(prefix + key, fallback).toInt(), low, high);
        };
        return {number("pitch", 0, -12, 12), number("effect", 0, 0, 2), number("gain", 100, 0, 200),
                number("gate", -60, -60, -20), settings.value(prefix + "preserve", true).toBool(),
                number("amount", 40, 0, 100), number("robotHz", 65, 20, 160), number("radioDrive", 25, 0, 100),
                number("bass", 0, -12, 12), number("treble", 0, -12, 12)};
    }
    void writeVoice(const QString &prefix, const VoiceProfile &v) {
        settings.setValue(prefix + "pitch", v.pitch); settings.setValue(prefix + "effect", v.effect);
        settings.setValue(prefix + "gain", v.gain); settings.setValue(prefix + "gate", v.gate);
        settings.setValue(prefix + "preserve", v.preserve); settings.setValue(prefix + "amount", v.amount);
        settings.setValue(prefix + "robotHz", v.robotHz); settings.setValue(prefix + "radioDrive", v.radioDrive);
        settings.setValue(prefix + "bass", v.bass); settings.setValue(prefix + "treble", v.treble);
    }
    void selectVoice(int id) {
        selectedVoice = id; presets->button(id)->setChecked(true);
        customPanel->setVisible(id == 6);
    }
    void editedVoice() {
        if (updatingVoice) return;
        customVoice = currentVoice(); selectVoice(6);
    }
    void applyVoice(const VoiceProfile &v, int id) {
        updatingVoice = true;
        pitch->setValue(v.pitch); gain->setValue(v.gain); gate->setValue(v.gate);
        preserve->setChecked(v.preserve); effectType->setCurrentIndex(v.effect);
        amount->setValue(v.amount); robotHz->setValue(v.robotHz); radioDrive->setValue(v.radioDrive);
        bass->setValue(v.bass); treble->setValue(v.treble);
        // Values equal to widget defaults do not emit changed signals.
        controls.pitch = v.pitch; controls.gain = v.gain; controls.gate = v.gate;
        controls.preserve = v.preserve; controls.effect = v.effect;
        controls.effectAmount = v.amount; controls.robotHz = v.robotHz; controls.radioDrive = v.radioDrive;
        controls.bass = v.bass; controls.treble = v.treble;
        updatingVoice = false; selectVoice(id);
    }
    void save() {
        if (preview) return;
        if (aiPanel) aiPanel->save();
        settings.setValue("input", microphone->currentData()); settings.setValue("output", headphones->currentData());
        writeVoice("", currentVoice()); writeVoice("custom/", customVoice);
        settings.setValue("voiceProfileVersion", 2); settings.setValue("selectedVoice", selectedVoice);
        settings.setValue("listenVolume", listenVolume->value());
        settings.setValue("geometry", saveGeometry());
        // Listening, mute and recording are never automatically enabled on launch.
    }
    void devices() {
        const auto previousInput = microphone->currentData().toString();
        const auto previousOutput = headphones->currentData().toString();
        const auto preferredInput = previousInput.isEmpty() ? lastInput : previousInput;
        const auto preferredOutput = previousOutput.isEmpty() ? lastOutput : previousOutput;
        microphone->clear(); headphones->clear();
        try {
            const auto inventory = enumerateAudioDevices();
            for (const auto &d : inventory.inputs) microphone->addItem(d.label, d.id);
            for (const auto &d : inventory.outputs) headphones->addItem(d.label, d.id);
            auto select = [](QComboBox *box, const QString &preferred, const QString &fallback) {
                int i = box->findData(preferred); if (i < 0) i = box->findData(fallback);
                if (i >= 0) box->setCurrentIndex(i);
            };
            select(microphone, preferredInput, inventory.defaultInput);
            select(headphones, preferredOutput, inventory.defaultOutput);
            start->setEnabled(microphone->count() > 0 && inventory.virtualReady);
            listen->setEnabled(headphones->count() > 0);
            setStatus(!inventory.virtualReady ? inventory.message : microphone->count()
                ? "Tudo pronto. Escolha sua voz e inicie." : "Nenhum microfone encontrado. Conecte um e atualize.");
        } catch (const std::exception &e) {
            start->setEnabled(false);
            setStatus(platformAudioError(), false, true);
            status->setToolTip(QString::fromUtf8(e.what()));
        }
    }
    void setRunning(bool running) {
        aiPanel->setRunning(running);
        microphone->setEnabled(!running); headphones->setEnabled(!running); refresh->setEnabled(!running);
        start->setText(running ? "■  Parar microfone virtual" : "▶  Iniciar microfone virtual");
    }
    void finished() {
        if (!worker) return;
        auto error = worker->error;
        worker->wait(); delete worker; worker = nullptr;
        if (changingVoice) {
            changingVoice = false; holdVirtualMic = true; controls.mute = previousMute;
#ifndef Q_OS_WIN
            audioReady = false;
#endif
            setRunning(false); start->setEnabled(microphone->count() > 0);
            setStatus("Voz pausada. Configure o modo escolhido e clique em Iniciar microfone virtual.");
            meter->setValue(0); return;
        }
        holdVirtualMic = false;
#ifndef Q_OS_WIN
        audioReady = false; updateGameMode(); discordRoute.reset();
        discordHint->setText("Inicie o microfone virtual para conectar o Discord.");
#endif
        virtualMic.destroy();
        setRunning(false); start->setEnabled(microphone->count() > 0);
        if (!error.isEmpty()) {
            setStatus("Áudio interrompido. " + error, false, true);
        } else if (!virtualMic.cleanupError.isEmpty()) {
            setStatus("Áudio parado; não foi possível remover a entrada virtual. Reiniciar o app tenta recuperá-la.", false, true);
        } else setStatus(platformStoppedMessage());
        meter->setValue(0);
    }
    void toggle() {
        if (worker) {
            start->setEnabled(false); setStatus("Parando…"); worker->requestInterruption(); return;
        }
        try {
            if (microphone->currentData().toString().isEmpty()) throw std::runtime_error("Selecione um microfone.");
            const auto ai = aiPanel->captureSettings();
            start->setEnabled(false); setStatus("Criando microfone virtual…");
            if (!holdVirtualMic) virtualMic.create();
            save();
            worker = new AudioWorker(controls, microphone->currentData().toString(), headphones->currentData().toString(), virtualMic.target(), ai);
            worker->onAiStatus = [this](QString message) {
                QMetaObject::invokeMethod(this, [this,message] { if(worker) aiPanel->setSpeechStatus(message); }, Qt::QueuedConnection);
            };
            worker->onReady = [this] {
                QMetaObject::invokeMethod(this, [this] {
                    if (!worker || worker->isInterruptionRequested()) return;
                    setStatus("Ao vivo • selecione " + discordSourceName() + " no Discord", true);
                    start->setEnabled(true);
#ifndef Q_OS_WIN
                    audioReady = true; updateGameMode(); updateDiscord();
                    // PipeWire can restore remap-source volume on first activation,
                    // after create(). Normalize only at startup, not continuously.
                    try { virtualMic.prepareLevel(); } catch (...) {}
                    QTimer::singleShot(500, this, [this] {
                        if (!audioReady) return;
                        try { virtualMic.prepareLevel(); }
                        catch (...) { setStatus("Confira o volume de JOCAS Microfone nas configurações de áudio.", true); }
                    });
#endif
                }, Qt::QueuedConnection);
            };
            connect(worker, &QThread::finished, this, [this] { finished(); });
            holdVirtualMic = false;
            setRunning(true); start->setEnabled(true); worker->start();
        } catch (const std::exception &e) {
            if (!holdVirtualMic) virtualMic.destroy();
            start->setEnabled(true); setRunning(false);
            setStatus("Não foi possível iniciar.", false, true);
            QMessageBox::warning(this, "Falha ao iniciar", QString::fromUtf8(e.what()));
        }
    }
public:
    explicit Window(bool screenshot = false) : preview(screenshot) {
        setWindowTitle("Modificador de Voz JOCAS");
#ifdef Q_OS_WIN
        setWindowIcon(QIcon(QCoreApplication::applicationDirPath() + "/jocas.ico"));
#else
        setWindowIcon(QIcon(QCoreApplication::applicationDirPath() + "/gepetas.svg"));
#endif
        setMinimumSize(680, 600); resize(800, screenshot ? (QCoreApplication::arguments().contains("--preview-custom") ? 2310 : 2100) : 900);
        auto *outer = new QVBoxLayout(this); outer->setContentsMargins(0, 0, 0, 0);
        auto *scroll = new QScrollArea; scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        auto *content = new QWidget; scroll->setWidget(content); outer->addWidget(scroll);
        auto *root = new QVBoxLayout(content); root->setContentsMargins(28, 22, 28, 22); root->setSpacing(14);
        root->setSizeConstraint(QLayout::SetMinimumSize);
        auto *header = new QHBoxLayout;
        auto *titles = new QVBoxLayout; titles->setSpacing(3);
        titles->addWidget(label("JOCAS", "brand"));
        auto *subtitle = label("Modificador de Voz • Sua voz. Outro personagem.", "subtitle"); subtitle->setWordWrap(false); titles->addWidget(subtitle);
        auto *badge = label(platformLabel(), "badge"); badge->setFixedHeight(34); badge->setAlignment(Qt::AlignCenter);
        header->addLayout(titles, 1); header->addWidget(badge); root->addLayout(header);
        auto *deviceCard = new QGroupBox("01  /  CONEXÕES"); auto *dev = new QGridLayout(deviceCard);
        dev->setContentsMargins(18, 25, 18, 18); dev->setVerticalSpacing(10);
        microphone = new QComboBox; headphones = new QComboBox;
        for (auto *box : {microphone, headphones}) { box->setMinimumHeight(36); box->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon); box->setMinimumContentsLength(16); }
        microphone->setAccessibleName("Microfone de entrada"); headphones->setAccessibleName("Fones para escuta");
        refresh = new QPushButton("Atualizar"); refresh->setFixedWidth(100);
        dev->setColumnStretch(0, 1); dev->setColumnStretch(1, 1);
        dev->addWidget(label("Seu microfone", "field"), 0, 0); dev->addWidget(label("Fones / saída de escuta", "field"), 0, 1);
        dev->addWidget(microphone, 1, 0); dev->addWidget(headphones, 1, 1); dev->addWidget(refresh, 1, 2); root->addWidget(deviceCard);
        connect(refresh, &QPushButton::clicked, this, [this] { devices(); });
#ifdef Q_OS_WIN
        auto *driverHelp = new QPushButton("Configurar microfone virtual no Windows (VB-CABLE)");
        connect(driverHelp, &QPushButton::clicked, this, [this] {
            QMessageBox::information(this, "Microfone virtual no Windows",
                "Instale o VB-CABLE pelo site oficial e reinicie o Windows se solicitado. Depois, clique em Atualizar.\n\n"
                "O JOCAS envia os efeitos para CABLE Input. No Discord, selecione CABLE Output como entrada. "
                "Não selecione CABLE Input como saída de som do Windows ou do Discord.");
            QDesktopServices::openUrl(QUrl("https://vb-audio.com/Cable/"));
        });
        root->addWidget(driverHelp);
#endif
        auto *voiceCard = new QGroupBox("02  /  SUA VOZ"); voiceCard->setMinimumHeight(330); auto *voices = new QVBoxLayout(voiceCard);
        voices->setContentsMargins(18, 26, 18, 18); voices->setSpacing(10);
        auto *presetArea = new QWidget; presetArea->setMinimumHeight(134);
        auto *grid = new QGridLayout(presetArea); grid->setContentsMargins(0, 0, 0, 0); grid->setSpacing(8);
        grid->setRowMinimumHeight(0, 62); grid->setRowMinimumHeight(1, 62);
        presets = new QButtonGroup(this); presets->setExclusive(true);
        const QStringList names{"Natural", "Grave", "Aguda", "Robô", "Rádio", "Monstro", "Personalizada"};
        const QStringList descriptions{"Sua voz original", "Grave clara • Discord", "Mais fina", "Voz metálica", "Efeito comunicador", "Grave + metálica", "Sua criação • ajustes salvos"};
        for (int i = 0; i < names.size(); ++i) {
            auto *b = new QPushButton(names[i] + "\n" + descriptions[i]); b->setCheckable(true); b->setMinimumHeight(62);
            b->setObjectName("preset"); presets->addButton(b, i); grid->addWidget(b, i / 3, i % 3, 1, i == 6 ? 3 : 1);
        }
        voices->addWidget(presetArea);
        voices->addWidget(label("Grave envia somente a voz processada, com tom e timbre mais baixos. Natural envia sua voz original. A intensidade do efeito extra não mistura o microfone original ao tom modificado."));
        pitch = slider(voices, "Tom da voz", -12, 12, 0, [](int v) { return QString(v > 0 ? "+%1 semitons" : "%1 semitons").arg(v); },
                       [this](int v) { controls.pitch = v; editedVoice(); });
        preserve = new QCheckBox("Preservar o timbre ao mudar o tom"); voices->addWidget(preserve);
        connect(preserve, &QCheckBox::toggled, this, [this](bool b) { controls.preserve = b; editedVoice(); });
        auto *adjustments = new QHBoxLayout; auto *left = new QVBoxLayout; auto *right = new QVBoxLayout;
        gain = slider(left, "Volume enviado", 0, 200, 100, [](int v) { return QString::number(v) + "%"; }, [this](int v) { controls.gain = v; editedVoice(); });
        gate = slider(right, "Corte de ruído baixo", -60, -20, -60, [](int v) { return v == -60 ? QString("Desligado") : QString::number(v) + " dB"; }, [this](int v) { controls.gate = v; if (gateHint) gateHint->setVisible(v > -40); editedVoice(); });
        adjustments->addLayout(left); adjustments->addSpacing(24); adjustments->addLayout(right); voices->addLayout(adjustments); root->addWidget(voiceCard);
        gateHint = label("Corte alto pode apagar palavras. Para chamadas, prefira Desligado ou um valor abaixo de −40 dB.");
        gateHint->setStyleSheet("color:#f4c389;"); gateHint->hide(); voices->addWidget(gateHint);
        customPanel = new QWidget; customPanel->setObjectName("customPanel");
        auto *custom = new QVBoxLayout(customPanel); custom->setContentsMargins(0, 6, 0, 0); custom->setSpacing(8);
        custom->addWidget(label("PERSONALIZADA • Seus ajustes ficam guardados ao trocar de voz e fechar o app."));
        effectType = new QComboBox; effectType->setAccessibleName("Efeito da voz personalizada");
        effectType->addItems({"Sem efeito extra", "Robô / metálico", "Rádio / comunicador"}); custom->addWidget(effectType);
        connect(effectType, &QComboBox::currentIndexChanged, this, [this](int v) {
            controls.effect = v; editedVoice();
        });
        auto percent = [](int v) { return QString::number(v) + "%"; };
        auto db = [](int v) { return QString(v > 0 ? "+%1 dB" : "%1 dB").arg(v); };
        amount = slider(custom, "Intensidade do efeito extra", 0, 100, 40, percent,
                        [this](int v) { controls.effectAmount = v; editedVoice(); });
        robotHz = slider(custom, "Frequência do robô", 20, 160, 65, [](int v) { return QString::number(v) + " Hz"; },
                         [this](int v) { controls.robotHz = v; editedVoice(); });
        radioDrive = slider(custom, "Distorção do rádio", 0, 100, 25, percent,
                            [this](int v) { controls.radioDrive = v; editedVoice(); });
        bass = slider(custom, "Graves", -12, 12, 0, db, [this](int v) { controls.bass = v; editedVoice(); });
        treble = slider(custom, "Agudos / clareza", -12, 12, 0, db, [this](int v) { controls.treble = v; editedVoice(); });
        auto updateEffectControls = [this](int effect) {
            amount->setEnabled(effect != 0); robotHz->setEnabled(effect == 1); radioDrive->setEnabled(effect == 2);
        };
        connect(effectType, &QComboBox::currentIndexChanged, this, updateEffectControls); updateEffectControls(0);
        voices->addWidget(customPanel); customPanel->hide();
        auto *clearSpeech = new QPushButton("Grave clara para Discord"); clearSpeech->setObjectName("clearSpeech");
        clearSpeech->setToolTip("Tom −5, timbre mais grave, volume 100% e corte desligado. Mantém sua voz Personalizada salva.");
        connect(clearSpeech, &QPushButton::clicked, this, [this] { applyVoice(voicePreset(1), 1); });
        voices->addWidget(clearSpeech);
        connect(presets, &QButtonGroup::idClicked, this, [this](int id) {
            applyVoice(id == 6 ? customVoice : voicePreset(id), id);
        });
        aiPanel = new AiPanel(settings, preview); root->addWidget(aiPanel);
        aiPanel->onModeChanged = [this] {
            if (!worker || changingVoice) return;
            changingVoice = true; previousMute = controls.mute; controls.mute = true;
            start->setEnabled(false); setStatus("Pausando a voz para trocar de modo…");
            worker->requestInterruption();
        };
        auto *listenCard = new QGroupBox("03  /  ESCUTAR OU NÃO"); auto *lis = new QVBoxLayout(listenCard);
        lis->setContentsMargins(18, 25, 18, 16); lis->setSpacing(8);
        listen = new QCheckBox("Escutar a voz enviada (efeitos ou IA)"); listen->setObjectName("listen"); lis->addWidget(listen);
        monitorHint = label("Escuta desligada. Sua voz continua sendo enviada ao Discord quando o app está ao vivo."); lis->addWidget(monitorHint);
        listenVolume = slider(lis, "Volume só nos seus fones", 0, 100, 50, [](int v) { return QString::number(v) + "%"; }, [this](int v) { controls.listenVolume = v; });
        listenVolume->setEnabled(false);
        connect(listen, &QCheckBox::toggled, this, [this](bool on) {
            controls.listen = on; listenVolume->setEnabled(on);
            monitorHint->setText(on ? "Use fones para evitar eco. A escuta tem um pequeno atraso para evitar cortes."
                                   : "Escuta desligada. Sua voz continua sendo enviada ao Discord quando o app está ao vivo.");
        }); root->addWidget(listenCard);
#ifndef Q_OS_WIN
        auto *discordCard = new QGroupBox("04  /  DISCORD"); auto *discordLayout = new QVBoxLayout(discordCard);
        discordLayout->setContentsMargins(18, 25, 18, 16);
        discordAuto = new QCheckBox("Conectar a voz modificada ao Discord automaticamente");
        discordAuto->setObjectName("discordAuto");
        discordAuto->setChecked(!preview && settings.value("discordAuto", true).toBool());
        discordLayout->addWidget(discordAuto);
        discordHint = label("Inicie o microfone virtual para conectar o Discord."); discordLayout->addWidget(discordHint);
        auto *connectDiscord = new QPushButton("Conectar Discord"); discordLayout->addWidget(connectDiscord);
        connect(connectDiscord, &QPushButton::clicked, this, [this] { discordAuto->setChecked(true); updateDiscord(true); });
        connect(discordAuto, &QCheckBox::toggled, this, [this](bool on) {
            if (!preview) { settings.setValue("discordAuto", on); settings.sync(); }
            if (on) updateDiscord(true);
            else discordHint->setText("Selecione JOCAS Microfone manualmente no Discord.");
        });
        auto *discordTimer = new QTimer(this); discordTimer->setInterval(1500);
        connect(discordTimer, &QTimer::timeout, this, [this] { updateDiscord(); }); discordTimer->start();
        root->addWidget(discordCard);
        auto *gamesCard = new QGroupBox("05  /  JOGOS"); auto *gamesLayout = new QVBoxLayout(gamesCard);
        gamesLayout->setContentsMargins(18, 25, 18, 16);
        games = new QCheckBox("Modo jogos: usar JOCAS como microfone padrão");
        games->setObjectName("gameMode"); games->setChecked(!preview && settings.value("gameMode", false).toBool());
        gamesLayout->addWidget(games);
        gameHint = label("No jogo, selecione JOCAS Microfone como entrada de voz."); gamesLayout->addWidget(gameHint);
        gamesLayout->addWidget(label("Afeta jogos e apps que usam a entrada padrão. Ao parar, o microfone anterior é restaurado. Escolhas explícitas no jogo precisam ser alteradas nele."));
        connect(games, &QCheckBox::toggled, this, [this](bool enabled) {
            if (!preview) { settings.setValue("gameMode", enabled); settings.sync(); }
            updateGameMode();
        });
        root->addWidget(gamesCard);
#else
        root->addWidget(label("Jogos: escolha CABLE Output como microfone nas opções de áudio. Inicie o JOCAS antes de abrir o jogo."));
#endif
        root->addStretch();
        auto *footerWidget = new QWidget; outer->addWidget(footerWidget);
        auto *footer = new QVBoxLayout(footerWidget); footer->setContentsMargins(28, 8, 28, 18); footer->setSpacing(10);
        auto *statusRow = new QHBoxLayout; statusDot = new QLabel("●"); status = label("Pronto para iniciar.", "status");
        statusRow->addWidget(statusDot); statusRow->addWidget(status, 1);
        mute = new QPushButton("Silenciar"); mute->setCheckable(true); mute->setToolTip("Silencia o áudio enviado e a escuta (Ctrl+M)");
        mute->setShortcut(QKeySequence("Ctrl+M")); statusRow->addWidget(mute);
        connect(mute, &QPushButton::toggled, this, [this](bool on) { controls.mute = on; mute->setText(on ? "Reativar voz" : "Silenciar"); }); footer->addLayout(statusRow);
        meter = new QProgressBar; meter->setRange(0, 100); meter->setValue(0); meter->setTextVisible(false); meter->setFixedHeight(6);
        meter->setAccessibleName("Nível da voz enviada"); footer->addWidget(meter);
        start = new QPushButton("▶  Iniciar microfone virtual"); start->setObjectName("primary"); start->setMinimumHeight(48);
        connect(start, &QPushButton::clicked, this, [this] { toggle(); }); footer->addWidget(start);
        aiPanel->onBusy = [this](bool busy) { if (!worker) start->setEnabled(!busy && microphone->count() > 0); };
        auto *help = label("No Discord: Configurações → Voz e vídeo → Dispositivo de entrada → " + discordSourceName() + ". Mantenha este app aberto. Se as palavras ainda sumirem, teste desativar a supressão de ruído/Krisp no Discord.");
        help->setAlignment(Qt::AlignCenter); help->setTextInteractionFlags(Qt::TextSelectableByMouse); footer->addWidget(help);
        setStyleSheet(R"(
            QWidget { background:#10141e; color:#edf1fa; font-family:"Noto Sans",sans-serif; font-size:13px; }
            QLabel#brand { font-size:29px; font-weight:800; letter-spacing:3px; color:#a4f6d5; }
            QLabel#subtitle { color:#bbc4d5; font-size:14px; }
            QLabel#badge { color:#9eaac2; background:#1c2332; border-radius:6px; padding:9px; font-size:10px; font-weight:700; }
            QLabel#caption { color:#aab6ca; font-size:12px; }
            QLabel#field { color:#d4dcec; font-size:12px; font-weight:600; }
            QLabel#value { color:#a4f6d5; font-size:12px; }
            QGroupBox { background:#171d2a; border:1px solid #2b3448; border-radius:12px; margin-top:10px; font-size:11px; font-weight:700; color:#9eaec7; }
            QGroupBox::title { subcontrol-origin:margin; left:16px; padding:0 6px; }
            QLineEdit, QSpinBox { background:#222b3c; border:1px solid #39465f; border-radius:5px; padding:7px; selection-background-color:#326451; }
            QWidget#aiLocal, QWidget#aiOnline, QWidget#aiOptions { background:transparent; }
            QGroupBox QLabel, QGroupBox QCheckBox, QWidget#customPanel { background:transparent; }
            QPushButton, QComboBox { background:#222b3c; border:1px solid #39465f; border-radius:7px; padding:9px 12px; }
            QPushButton:hover, QComboBox:hover { border-color:#83dfbc; background:#29364a; }
            QPushButton:focus, QComboBox:focus { border:2px solid #a4f6d5; }
            QPushButton:checked { background:#204a42; border-color:#8ae7c2; color:#c6ffe7; }
            QPushButton#preset { font-size:12px; }
            QPushButton#primary { background:#9defcd; color:#10251e; font-size:15px; font-weight:750; border:0; }
            QPushButton#primary:hover { background:#c1ffe5; }
            QPushButton:disabled, QComboBox:disabled { color:#6f7c91; background:#1b2230; border-color:#2b3448; }
            QComboBox QAbstractItemView { background:#222b3c; selection-background-color:#326451; }
            QCheckBox { spacing:10px; } QCheckBox#listen { font-size:14px; font-weight:600; }
            QCheckBox::indicator { width:18px; height:18px; border:1px solid #73839f; border-radius:4px; background:#10141e; }
            QCheckBox::indicator:checked { background:#85e3bb; border:3px solid #3b8b70; }
            QSlider { background:transparent; } QSlider::groove:horizontal { height:4px; background:#344057; border-radius:2px; }
            QSlider::sub-page:horizontal { background:#80d8b3; border-radius:2px; }
            QSlider::handle:horizontal { background:#b0f6d9; width:14px; margin:-5px 0; border-radius:7px; }
            QSlider::handle:horizontal:disabled { background:#58657b; }
            QProgressBar { background:#293246; border:0; border-radius:3px; } QProgressBar::chunk { background:#86e3bd; border-radius:3px; }
            QToolTip { background:#263246; color:#fff; padding:6px; border:1px solid #66758b; }
        )");
        auto *timer = new QTimer(this); timer->setInterval(50);
        connect(timer, &QTimer::timeout, this, [this] {
            if (interrupted) { close(); return; }
            const float v = controls.level;
            meter->setValue(v <= .00001f ? 0 : std::clamp(int((20 * std::log10(v) + 60) / 60 * 100), 0, 100));
        }); timer->start();
        if (preview) {
            microphone->addItem("Microfone USB"); headphones->addItem("Fones de ouvido");
            applyVoice(voicePreset(0), QCoreApplication::arguments().contains("--preview-custom") ? 6 : 0); setStatus("Tudo pronto. Escolha sua voz e inicie.");
        } else {
            lastInput = settings.value("input").toString(); lastOutput = settings.value("output").toString();
            auto voice = settings.contains("pitch") ? readVoice("") : voicePreset(1);
            const bool migrate = settings.value("voiceProfileVersion", 0).toInt() < 2;
            if (migrate && settings.contains("pitch")) {
                writeVoice("beforeClarityUpdate/", voice);
                voice.gate = -60; voice.preserve = true; voice.gain = std::min(voice.gain, 100);
                voice.pitch = std::clamp(voice.pitch, -5, 5);
                customVoice = voice;
            } else customVoice = settings.contains("custom/pitch") ? readVoice("custom/") : voice;
            applyVoice(voice, migrate ? (settings.contains("pitch") ? 6 : 1) : std::clamp(settings.value("selectedVoice", 6).toInt(), 0, 6));
            listenVolume->setValue(settings.value("listenVolume", 50).toInt());
            if (settings.contains("geometry")) restoreGeometry(settings.value("geometry").toByteArray());
            QTimer::singleShot(0, this, [this] { devices(); });
        }
    }
    ~Window() override {
#ifndef Q_OS_WIN
        audioReady = false; updateGameMode();
#endif
        save();
        if (worker) { disconnect(worker, nullptr, this, nullptr); worker->requestInterruption(); worker->wait(); delete worker; }
        virtualMic.destroy();
    }
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setApplicationName("Gepetas"); app.setOrganizationName("Gepetas"); app.setApplicationVersion("1.4.2");
    app.setApplicationDisplayName("Modificador de Voz JOCAS");
    app.setDesktopFileName("gepetas");
    const bool screenshot = app.arguments().contains("--screenshot");
    QLockFile lock(lockPath());
    if (!screenshot && !lock.tryLock(0)) {
        QMessageBox::information(nullptr, "Modificador de Voz JOCAS", lock.error() == QLockFile::LockFailedError
            ? "O Modificador de Voz JOCAS já está aberto. Use a janela existente."
            : "Não foi possível acessar a pasta de configuração do JOCAS. Abra o app como seu usuário, sem sudo."); return 1;
    }
    std::signal(SIGINT, handleSignal); std::signal(SIGTERM, handleSignal);
    Window window(screenshot); window.show();
    if (screenshot) {
        const int i = app.arguments().indexOf("--screenshot");
        const QString path = app.arguments().value(i + 1, "gepetas-preview.png");
        QTimer::singleShot(250, &window, [&, path] { app.exit(window.grab().save(path) ? 0 : 1); });
    }
    return app.exec();
}
