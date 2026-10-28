// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "ai_speech.hpp"
#include <QtWidgets>
#include <QUuid>

class AiPanel : public QGroupBox {
    QSettings &settings;
    bool preview;
    QButtonGroup *modes;
    QWidget *local, *online, *options;
    QComboBox *voices, *apiVoice;
    QLineEdit *recognizer, *key, *customId, *style;
    QSpinBox *speaker;
    QSlider *speed, *variation, *sensitivity;
    QCheckBox *consent;
    QLabel *hint, *runtimeHint;
    QPushButton *prepare;
    QProcess *job = nullptr;
    int stage = 0;
    QByteArray messages;
    QString jobError;
    bool jobReportedError = false;
    bool running = false;
    int selectedMode = 0;
    QString voicePath(const QString &id) const {
        return AiPaths::root() + "/models/voices/" + id + "/pt_BR-" + id + "-medium.onnx";
    }
    static QLabel *text(const QString &s) { auto *w = new QLabel(s); w->setWordWrap(true); return w; }
    void selectMode(int id) {
        local->setVisible(id == 1); online->setVisible(id == 2); options->setVisible(id != 0);
        hint->setText(id == 0 ? "Efeitos de voz em tempo real. Para voz sintetizada, escolha um dos modos de IA."
            : "Voz gerada por IA. O app reconhece e repete suas palavras, sem responder a elas. Faça pausas: há atraso por frase e pode haver erros de transcrição. Use fones.");
    }
    void refreshVoices(const QString &preferred = {}) {
        const QString wanted = preferred.isEmpty() ? voices->currentData().toString() : preferred;
        voices->clear();
        for (const auto &id : {QString("cadu"), QString("jeff")}) {
            const auto p = voicePath(id);
            voices->addItem(id.left(1).toUpper()+id.mid(1) + " • Português BR" + (QFileInfo::exists(p) ? "" : " (baixar)"),p);
        }
        QDir imports(AiPaths::root() + "/models/voices/importadas");
        for (const auto &p : imports.entryInfoList({"*.onnx"}, QDir::Files, QDir::Name))
            voices->addItem(p.completeBaseName().section("--",0,0) + " • importada", p.absoluteFilePath());
        const int i = voices->findData(wanted); if (i >= 0) voices->setCurrentIndex(i);
        runtimeHint->setText(QFileInfo::exists(AiPaths::root()+"/ready-1")
            ? "Motor de IA instalado. Modelos e preferências ficam guardados entre atualizações."
            : "Prepare o motor uma vez (requer internet e espaço livre). Depois baixe ou importe os modelos.");
    }
    void importVoice() {
        const QString p = QFileDialog::getOpenFileName(this,"Adicionar voz Piper treinada",{},"Voz Piper (*.onnx)");
        if (p.isEmpty()) return;
        QString json = p+".json";
        if (!QFileInfo::exists(json)) json = QFileDialog::getOpenFileName(this,"Selecione a configuração dessa voz",QFileInfo(p).absolutePath(),"Configuração Piper (*.json)");
        QFile f(json);
        if (!f.open(QIODevice::ReadOnly)) { hint->setText("A voz precisa do arquivo .onnx.json correspondente."); return; }
        const auto data = f.readAll(); const auto cfg = QJsonDocument::fromJson(data).object();
        if (!cfg.contains("audio") || !cfg.contains("phoneme_id_map") || cfg.value("num_speakers").toInt() < 1) {
            hint->setText("O arquivo selecionado não é uma configuração de voz Piper válida."); return;
        }
        const auto phonemes = cfg.value("phoneme_type").toString("espeak");
        if (phonemes != "espeak" && phonemes != "text") {
            hint->setText("Use uma voz Piper eSpeak ou text para funcionar sem downloads adicionais durante a fala."); return;
        }
        const auto dir = AiPaths::root()+"/models/voices/importadas"; QDir().mkpath(dir);
        QString name = QFileInfo(p).completeBaseName(); name.replace(QRegularExpression("[^\\p{L}\\p{N} ._-]"),"_");
        name = name.trimmed(); while (name.startsWith('.')) name.remove(0,1); if (name.isEmpty()) name="Minha voz";
        const auto dest = dir+"/"+name.left(60)+"--"+QUuid::createUuid().toString(QUuid::Id128).left(8)+".onnx";
        if (!QFile::copy(p,dest)) { hint->setText("Não foi possível copiar a voz. Confira o espaço livre."); return; }
        QFile out(dest+".json");
        if (!out.open(QIODevice::WriteOnly) || out.write(data) != data.size()) {
            out.close(); QFile::remove(dest); QFile::remove(dest+".json"); hint->setText("Não foi possível salvar a configuração da voz."); return;
        }
        out.close(); refreshVoices(dest);
        hint->setText("Voz adicionada. Se o modelo tiver vários falantes, escolha o número abaixo. Para voz simples, use 0.");
    }
    void startJob(QString program, QStringList args) {
        job = new QProcess(this); messages.clear(); jobError.clear(); jobReportedError = false;
        job->setProcessEnvironment(AiPaths::environment());
        connect(job,&QProcess::readyReadStandardError,this,[this]{
            // Only installation logs, never the speech worker or API key.
            jobError += QString::fromUtf8(job->readAllStandardError()).right(3000); jobError = jobError.right(4000);
        });
        connect(job,&QProcess::readyReadStandardOutput,this,[this]{
            messages += job->readAllStandardOutput();
            qsizetype n;
            while ((n=messages.indexOf('\n'))>=0) {
                const auto o=QJsonDocument::fromJson(messages.left(n)).object(); messages.remove(0,n+1);
                if (o.contains("message")) hint->setText(o.value("message").toString());
                if (o.value("type").toString()=="fatal") jobReportedError=true;
            }
        });
        connect(job,&QProcess::errorOccurred,this,[this](QProcess::ProcessError e){
            if (e==QProcess::FailedToStart) finishJob(false);
        });
        connect(job,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus s){
            finishJob(code==0 && s==QProcess::NormalExit && !jobReportedError);
        });
        setBusy(true); job->start(program,args);
    }
    void setBusy(bool busy) {
        local->setEnabled(!busy && !running); online->setEnabled(!busy && !running);
        options->setEnabled(!busy && !running); prepare->setEnabled(!busy && !running);
        for (auto *b:modes->buttons()) b->setEnabled(!busy);
        if (onBusy) onBusy(busy);
    }
    void finishJob(bool success) {
        if (!job) return;
        job->deleteLater(); job=nullptr;
        if (success && stage==1) {
            stage=2; hint->setText("Instalando reconhecimento e vozes de IA… Isso pode levar alguns minutos.");
            startJob(AiPaths::uv(),{"pip","install","--python",AiPaths::python(),"--require-hashes","-r",QCoreApplication::applicationDirPath()+"/ai/requirements.lock"}); return;
        }
        if (success && stage==2) { QFile flag(AiPaths::root()+"/ready-1"); if (flag.open(QIODevice::WriteOnly)) flag.write("1\n"); }
        if (!success) {
            hint->setText(stage==3 ? "Download incompleto. Confira a internet e tente novamente; os arquivos completos são preservados."
                                  : "Não foi possível preparar a IA. Confira a internet e o espaço livre, depois tente novamente.");
            hint->setToolTip(jobError);
        } else if (stage!=3) hint->setText("Motor pronto. Em Usar SEM API, baixe a voz e o reconhecimento ou adicione seus modelos.");
        stage=0; refreshVoices(); setBusy(false);
    }
    void download() {
        if (!QFileInfo::exists(AiPaths::root()+"/ready-1")) { hint->setText("Clique em Preparar IA antes de baixar os modelos."); return; }
        QString id = voices->currentData().toString()==voicePath("jeff") ? "jeff" : "cadu";
        stage=3; startJob(AiPaths::python(),{AiPaths::helper(),"download","--root",AiPaths::root(),"--voice",id,"--recognizer"});
    }
public:
    std::function<void(bool)> onBusy;
    std::function<void()> onModeChanged;
    explicit AiPanel(QSettings &prefs, bool screenshot, QWidget *parent=nullptr)
        : QGroupBox("VOZES POR IA",parent), settings(prefs), preview(screenshot) {
        setObjectName("aiPanel");
        auto *layout=new QVBoxLayout(this); layout->setContentsMargins(18,25,18,16); layout->setSpacing(10);
        auto *row=new QHBoxLayout; modes=new QButtonGroup(this);
        const QStringList titles={"Efeitos atuais","Usar SEM API","Ativar por API"};
        for(int i=0;i<3;++i){auto *b=new QPushButton(titles[i]); b->setCheckable(true); modes->addButton(b,i);row->addWidget(b);}
        layout->addLayout(row);
        local=new QWidget; local->setObjectName("aiLocal"); auto *l=new QVBoxLayout(local); l->setContentsMargins(0,0,0,0);
        voices=new QComboBox; voices->setAccessibleName("Voz local de IA"); l->addWidget(text("Voz local (Piper .onnx + .onnx.json)")); l->addWidget(voices);
        auto *buttons=new QHBoxLayout; auto *get=new QPushButton("Baixar voz + reconhecimento"); auto *add=new QPushButton("Adicionar minha voz");
        buttons->addWidget(get);buttons->addWidget(add);l->addLayout(buttons);
        l->addWidget(text("Aceita modelos Piper eSpeak/text já treinados, próprios ou de terceiros. Uma gravação .wav ou um modelo RVC não substitui esses arquivos. O app não treina nem clona vozes."));
        l->addWidget(text("Reconhecimento local • pasta Faster Whisper multilíngue"));
        recognizer=new QLineEdit;recognizer->setReadOnly(true);recognizer->setAccessibleName("Pasta do reconhecimento local");
        auto *recRow=new QHBoxLayout;auto *manual=new QPushButton("Adicionar pasta"); recRow->addWidget(recognizer,1);recRow->addWidget(manual);l->addLayout(recRow);
        auto *modelLink=new QLabel("Download manual: <a href='https://huggingface.co/rhasspy/piper-voices/tree/main/pt/pt_BR'>vozes Piper</a> · <a href='https://huggingface.co/Systran/faster-whisper-base/tree/main'>reconhecimento Base</a>");
        modelLink->setStyleSheet("QLabel a { color:#a4f6d5; }");modelLink->setText(modelLink->text().replace("<a href=", "<a style='color:#a4f6d5' href="));modelLink->setOpenExternalLinks(true);modelLink->setWordWrap(true);l->addWidget(modelLink);
        auto *speakerRow=new QHBoxLayout;speakerRow->addWidget(text("Falante do modelo (voz simples: 0)"));speaker=new QSpinBox;speaker->setRange(0,10000);speakerRow->addWidget(speaker);l->addLayout(speakerRow);
        l->addWidget(text("Variação da voz local"));variation=new QSlider(Qt::Horizontal);variation->setRange(0,100);variation->setValue(67);variation->setAccessibleName("Variação da voz local");l->addWidget(variation);layout->addWidget(local);
        online=new QWidget; online->setObjectName("aiOnline");auto *o=new QVBoxLayout(online);o->setContentsMargins(0,0,0,0);
        o->addWidget(text("API OpenAI • sua chave é usada somente nesta sessão e não é salva."));
        key=new QLineEdit; key->setEchoMode(QLineEdit::Password);key->setPlaceholderText("Cole sua chave de API");key->setAccessibleName("Chave da API OpenAI");o->addWidget(key);
        apiVoice=new QComboBox;apiVoice->setAccessibleName("Voz de IA por API");apiVoice->addItems({"marin","cedar","alloy","ash","ballad","coral","echo","fable","nova","onyx","sage","shimmer","verse"});o->addWidget(apiVoice);
        o->addWidget(text("Rio, do ChatGPT, não está na lista pública da API de fala. As vozes acima são opções próprias da API e podem soar diferentes."));
        customId=new QLineEdit;customId->setPlaceholderText("ID de voz própria opcional (voice_…); substitui a lista");customId->setAccessibleName("ID de voz própria na API");o->addWidget(customId);
        style=new QLineEdit;style->setMaxLength(600);style->setPlaceholderText("Estilo da voz: grave, clara, português brasileiro…");style->setAccessibleName("Instruções de estilo da voz");o->addWidget(style);
        o->addWidget(text("Use um ID de voz personalizada já disponível na sua conta da API. Esta tela não cria uma voz na conta. A API tem cobrança separada do ChatGPT."));
        consent=new QCheckBox("Autorizo enviar minhas frases e o texto reconhecido à OpenAI nesta sessão");consent->setObjectName("aiConsent");o->addWidget(consent);layout->addWidget(online);
        options=new QWidget; options->setObjectName("aiOptions");auto *opts=new QVBoxLayout(options);opts->setContentsMargins(0,0,0,0);
        auto *speedLabel=text("Velocidade da voz: 100%");opts->addWidget(speedLabel);speed=new QSlider(Qt::Horizontal);speed->setRange(70,140);speed->setValue(100);speed->setAccessibleName("Velocidade da voz de IA");opts->addWidget(speed);
        connect(speed,&QSlider::valueChanged,this,[speedLabel](int v){speedLabel->setText(QString("Velocidade da voz: %1%").arg(v));});
        auto *senseLabel=text("Detecção de fala: −42 dB");opts->addWidget(senseLabel);sensitivity=new QSlider(Qt::Horizontal);sensitivity->setRange(-55,-25);sensitivity->setValue(-42);sensitivity->setAccessibleName("Sensibilidade de detecção de fala");opts->addWidget(sensitivity);
        connect(sensitivity,&QSlider::valueChanged,this,[senseLabel](int v){senseLabel->setText(QString("Detecção de fala: %1 dB • menor valor capta sons mais baixos").arg(v));});
        opts->addWidget(text("No modo IA, os efeitos de tom/robô/rádio acima não são aplicados; o volume geral continua valendo. O microfone original nunca é misturado à voz gerada."));
        prepare=new QPushButton("Preparar IA (primeiro uso)");opts->addWidget(prepare);runtimeHint=text("");opts->addWidget(runtimeHint);layout->addWidget(options);
        hint=text("");hint->setObjectName("aiStatus");hint->setTextInteractionFlags(Qt::TextSelectableByMouse);layout->addWidget(hint);
        recognizer->setText(preview?AiPaths::root()+"/models/recognizer/base":settings.value("ai/recognizer",AiPaths::root()+"/models/recognizer/base").toString());
        refreshVoices(preview?QString():settings.value("ai/voice").toString());
        if (!preview) {
            speed->setValue(settings.value("ai/speed",100).toInt());variation->setValue(settings.value("ai/variation",67).toInt());
            sensitivity->setValue(settings.value("ai/sensitivity",-42).toInt());speaker->setValue(settings.value("ai/speaker",0).toInt());
            apiVoice->setCurrentText(settings.value("ai/apiVoice","marin").toString());customId->setText(settings.value("ai/customId").toString());style->setText(settings.value("ai/style").toString());
        }
        const int id=preview?1:std::clamp(settings.value("ai/mode",0).toInt(),0,2);modes->button(id)->setChecked(true);selectedMode=id;selectMode(id);
        connect(modes,&QButtonGroup::idClicked,this,[this](int id){
            if(id==selectedMode)return;
            selectedMode=id;selectMode(id);
            if(onModeChanged)onModeChanged();
        });
        connect(add,&QPushButton::clicked,this,[this]{importVoice();});connect(get,&QPushButton::clicked,this,[this]{download();});
        connect(manual,&QPushButton::clicked,this,[this]{
            const auto p=QFileDialog::getExistingDirectory(this,"Pasta Faster Whisper multilíngue");if(p.isEmpty())return;
            for(const auto &n:{"model.bin","config.json","tokenizer.json","vocabulary.txt"})if(!QFileInfo::exists(p+"/"+n)){hint->setText("A pasta precisa conter model.bin, config.json, tokenizer.json e vocabulary.txt.");return;}
            recognizer->setText(p);hint->setText("Reconhecimento manual selecionado. Nenhum download será feito ao falar.");
        });
        connect(prepare,&QPushButton::clicked,this,[this]{
            stage=1;hint->setText("Preparando Python e motor de IA… Isso pode levar alguns minutos.");QDir().mkpath(AiPaths::root());
            startJob(AiPaths::uv(),{"venv","--allow-existing","--python","3.12","--managed-python",AiPaths::root()+"/venv"});
        });
    }
    ~AiPanel() override { if(job){job->disconnect(this);job->kill();job->waitForFinished(2000);} }
    void save() {
        if(preview)return;
        settings.setValue("ai/mode",modes->checkedId());settings.setValue("ai/voice",voices->currentData());settings.setValue("ai/recognizer",recognizer->text());
        settings.setValue("ai/speed",speed->value());settings.setValue("ai/variation",variation->value());settings.setValue("ai/speaker",speaker->value());
        settings.setValue("ai/sensitivity",sensitivity->value());settings.setValue("ai/apiVoice",apiVoice->currentText());settings.setValue("ai/customId",customId->text());settings.setValue("ai/style",style->text());
        // Key and online consent deliberately never enter QSettings.
    }
    void setRunning(bool value) { running=value;setBusy(job!=nullptr); }
    void setSpeechStatus(const QString &s) { hint->setText(s); }
    AiSettings captureSettings() const {
        AiSettings result;result.enabled=modes->checkedId()!=0;if(!result.enabled)return result;
        if(job)throw std::runtime_error("Aguarde o preparo ou download da IA terminar.");
        if(!QFileInfo::exists(AiPaths::python()) || !QFileInfo::exists(AiPaths::root()+"/ready-1") || !QFileInfo::exists(AiPaths::helper()))throw std::runtime_error("Clique em Preparar IA antes de iniciar esse modo.");
        result.config={{"mode",modes->checkedId()==2?"api":"local"},{"voicePath",voices->currentData().toString()},
            {"recognizer",recognizer->text()},{"speed",speed->value()/100.},{"variation",variation->value()/100.},
            {"speaker",speaker->value()},{"sensitivity",sensitivity->value()},{"apiVoice",apiVoice->currentText()},
            {"customVoiceId",customId->text().trimmed()},{"style",style->text()},{"onlineConsent",consent->isChecked()}};
        if(modes->checkedId()==1) {
            const auto p=voices->currentData().toString();
            if(!QFileInfo::exists(p)||!QFileInfo::exists(p+".json"))throw std::runtime_error("Baixe uma voz ou adicione um modelo Piper antes de iniciar.");
            for(const auto &n:{"model.bin","config.json","tokenizer.json","vocabulary.txt"})
                if(!QFileInfo::exists(recognizer->text()+"/"+n))throw std::runtime_error("Baixe o reconhecimento ou adicione a pasta do modelo.");
        } else {
            if(key->text().trimmed().isEmpty())throw std::runtime_error("Digite sua chave da API nesta sessão.");
            if(!consent->isChecked())throw std::runtime_error("Autorize o envio de áudio à OpenAI para usar a API.");
            result.key=key->text().trimmed();
        }
        return result;
    }
};
