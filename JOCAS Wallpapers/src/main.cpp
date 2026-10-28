// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtWidgets>
#include <QSharedMemory>
#include <QCloseEvent>
#include <QFileSystemWatcher>
#include <QProcess>

static QString dataHome() { return qEnvironmentVariable("XDG_DATA_HOME", QDir::homePath()+"/.local/share"); }
static QJsonObject readJson(const QString &path) {
    QFile f(path); if (!f.open(QIODevice::ReadOnly) || f.size()>16*1024*1024) return {};
    return QJsonDocument::fromJson(f.readAll()).object();
}
static QByteArray command(const QString &exe, const QStringList &args) {
    QProcess p; p.start(exe,args); if (!p.waitForFinished(8000) || p.exitCode()!=0) return {};
    return p.readAllStandardOutput();
}
struct Wallpaper { QString id, title, path, type, preview; bool ready=false; };
struct Library { QString root, assets; };
static QList<Library> libraries() {
    QStringList roots{dataHome()+"/Steam", QDir::homePath()+"/.steam/steam",
        QDir::homePath()+"/.var/app/com.valvesoftware.Steam/.local/share/Steam",
        QDir::homePath()+"/.var/app/com.valvesoftware.Steam/data/Steam"};
    QSettings config("JOCAS","Wallpapers"); roots << config.value("extraLibrary").toStringList();
    for (const auto &root:QStringList(roots)) {
        QFile f(root+"/steamapps/libraryfolders.vdf");
        if (!f.open(QIODevice::ReadOnly)) continue;
        auto it=QRegularExpression("\"path\"\\s+\"([^\"]+)\"").globalMatch(QString::fromUtf8(f.readAll()));
        while(it.hasNext()) roots << it.next().captured(1).replace("\\\\","\\");
    }
    QSet<QString> seen; QList<Library> result;
    for(auto root:roots) {
        root=QFileInfo(root).canonicalFilePath(); if(root.isEmpty()||seen.contains(root))continue;
        seen.insert(root); result.append({root,root+"/steamapps/common/wallpaper_engine/assets"});
    }
    return result;
}
static QList<Wallpaper> scan(const QList<Library> &libs) {
    QList<Wallpaper> result; QSet<QString> seen;
    for(const auto &lib:libs) {
        QDir dir(lib.root+"/steamapps/workshop/content/431960");
        for(const auto &id:dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot)) {
            QString path=QFileInfo(dir.filePath(id)).canonicalFilePath();
            if(seen.contains(path))continue;
            seen.insert(path);
            auto d=readJson(path+"/project.json"); if(d.isEmpty())continue;
            QString type=d["type"].toString().toLower(), file=d["file"].toString();
            bool ready=QStringList{"scene","video","web"}.contains(type)&&!file.isEmpty();
            // Scene JSON can be stored inside scene.pkg rather than as a loose file.
            ready=ready&&(QFile::exists(path+"/"+file)||(type=="scene"&&QFile::exists(path+"/scene.pkg")));
            result.append({id,d["title"].toString(id),path,type,path+"/"+d["preview"].toString("preview.jpg"),ready});
        }
    }
    std::sort(result.begin(),result.end(),[](const auto&a,const auto&b){return a.title.localeAwareCompare(b.title)<0;});
    return result;
}
static QJsonArray outputs() {return QJsonDocument::fromJson(command("kscreen-doctor",{"-j"})).object()["outputs"].toArray();}

class Window:public QWidget {
    QSettings settings{"JOCAS","Wallpapers"};
    QList<Library> libs; QList<Wallpaper> wallpapers;
    QListWidget *list=new QListWidget; QLineEdit *search=new QLineEdit;
    QComboBox *screen=new QComboBox; QSpinBox *fps=new QSpinBox;
    QCheckBox *mute=new QCheckBox("Sem som"), *login=new QCheckBox("Iniciar com o CachyOS"),
        *follow=new QCheckBox("Acompanhar seleção no Wallpaper Engine");
    QLabel *status=new QLabel, *count=new QLabel; QSystemTrayIcon *tray=nullptr;
    QMap<QString,QProcess*> players; QSet<QProcess*> stopping; QMap<QString,QString> applied;
    QString base=QCoreApplication::applicationDirPath();
    QString steamConfig, steamDigest; bool quitting=false;
public:
    Window() {
        setWindowTitle("JOCAS Wallpapers • CachyOS"); resize(1020,720);
        auto layout=new QVBoxLayout(this); layout->setContentsMargins(24,24,24,20); layout->setSpacing(14);
        auto title=new QLabel("JOCAS Wallpapers"); QFont font=title->font();font.setPointSize(24);font.setBold(true);title->setFont(font);
        layout->addWidget(title);layout->addWidget(new QLabel("Baixe pela Steam e aplique seus wallpapers no KDE."));
        auto steamButtons=new QHBoxLayout;
        auto button=[&](QString label,auto callback){auto b=new QPushButton(label);connect(b,&QPushButton::clicked,this,callback);return b;};
        steamButtons->addWidget(button("Abrir Wallpaper Engine",[]{QDesktopServices::openUrl(QUrl("steam://rungameid/431960"));}));
        steamButtons->addWidget(button("Workshop da Steam",[]{QDesktopServices::openUrl(QUrl("steam://openurl/https://steamcommunity.com/app/431960/workshop/"));}));
        steamButtons->addWidget(button("Atualizar lista",[this]{reload();}));
        steamButtons->addWidget(button("Adicionar biblioteca…",[this]{
            auto dir=QFileDialog::getExistingDirectory(this,"Escolha a pasta que contém steamapps");if(dir.isEmpty())return;
            if(!QDir(dir+"/steamapps").exists()){QMessageBox::information(this,"Biblioteca Steam","Escolha a pasta que contém a subpasta steamapps.");return;}
            auto dirs=settings.value("extraLibrary").toStringList();dirs<<dir;dirs.removeDuplicates();settings.setValue("extraLibrary",dirs);reload();
        }));layout->addLayout(steamButtons);
        search->setPlaceholderText("Buscar wallpaper por nome, tipo ou ID…"); layout->addWidget(search);layout->addWidget(count);
        list->setIconSize(QSize(132,76)); list->setSpacing(4);layout->addWidget(list,1);
        auto controls=new QHBoxLayout; controls->addWidget(new QLabel("Aplicar em:"));controls->addWidget(screen,1);
        controls->addWidget(new QLabel("FPS:"));fps->setRange(10,60);fps->setValue(settings.value("fps",30).toInt());controls->addWidget(fps);
        mute->setChecked(settings.value("mute",true).toBool());controls->addWidget(mute);layout->addLayout(controls);
        auto options=new QHBoxLayout;login->setChecked(settings.value("login",true).toBool());follow->setChecked(settings.value("follow",true).toBool());
        options->addWidget(login);options->addWidget(follow);options->addStretch();layout->addLayout(options);
        auto actions=new QHBoxLayout; auto applyButton=button("Aplicar wallpaper",[this]{applySelected();});
        applyButton->setDefault(true);actions->addWidget(applyButton);
        actions->addWidget(button("Parar animação",[this]{stop(true);}));
        actions->addWidget(button("Restaurar fundo anterior",[this]{restoreDesktop();}));layout->addLayout(actions);
        status->setWordWrap(true);status->setTextInteractionFlags(Qt::TextSelectableByMouse);layout->addWidget(status);
        connect(search,&QLineEdit::textChanged,this,[this](const QString&q){for(int i=0;i<list->count();++i)list->item(i)->setHidden(!list->item(i)->text().contains(q,Qt::CaseInsensitive));});
        connect(list,&QListWidget::itemDoubleClicked,this,[this]{applySelected();});
        for(auto box:{login,follow})connect(box,&QCheckBox::toggled,this,[this]{saveOptions();});
        connect(mute,&QCheckBox::toggled,this,[this]{saveOptions();});
        connect(fps,&QSpinBox::valueChanged,this,[this]{saveOptions();});
        screen->addItem("Todos os monitores","*");
        for(auto value:outputs()){auto o=value.toObject();if(o["enabled"].toBool())screen->addItem(o["name"].toString(),o["name"].toString());}
        screen->setCurrentIndex(qMax(0,screen->findData(settings.value("screen","*"))));
        if(QSystemTrayIcon::isSystemTrayAvailable()) {
            tray=new QSystemTrayIcon(QIcon::fromTheme("preferences-desktop-wallpaper"),this);tray->setToolTip("JOCAS Wallpapers");
            auto menu=new QMenu(this);menu->addAction("Abrir JOCAS Wallpapers",this,[this]{show();raise();activateWindow();});
            menu->addAction("Parar animação",this,[this]{stop(true);});
            menu->addAction("Sair",this,[this]{quitting=true;stop(false);qApp->quit();});tray->setContextMenu(menu);
            connect(tray,&QSystemTrayIcon::activated,this,[this](auto reason){if(reason==QSystemTrayIcon::Trigger){show();raise();activateWindow();}});tray->show();
        }
        reload();
        QTimer *timer=new QTimer(this);timer->setInterval(4000);connect(timer,&QTimer::timeout,this,[this]{
            int total=0;for(const auto&lib:libs)total+=QDir(lib.root+"/steamapps/workshop/content/431960").entryList(QDir::Dirs|QDir::NoDotAndDotDot).size();
            if(total!=folderCount)reload();
            if(follow->isChecked())syncSteam();
        });timer->start();
    }
    int folderCount=0;
    void applyById(const QString &id,const QString &output) {
        for(int i=0;i<list->count();++i)if(wallpapers[list->item(i)->data(Qt::UserRole).toInt()].id==id){
            list->setCurrentRow(i);screen->setCurrentIndex(qMax(0,screen->findData(output)));applySelected();return;
        }
        status->setText("Wallpaper não encontrado na biblioteca Steam: "+id);
    }
    void reload() {
        libs=libraries();wallpapers=scan(libs);list->clear();folderCount=0;
        for(const auto &lib:libs)folderCount+=QDir(lib.root+"/steamapps/workshop/content/431960").entryList(QDir::Dirs|QDir::NoDotAndDotDot).size();
        for(int i=0;i<wallpapers.size();++i){const auto&w=wallpapers[i];
            QString type=w.type=="scene"?"Cena animada":w.type=="video"?"Vídeo":w.type=="web"?"Web":"Não disponível";
            auto item=new QListWidgetItem(QIcon(w.preview),w.title+"\n"+type+"  •  "+w.id+(w.ready?"":"  •  download incompleto / formato não suportado"),list);
            item->setData(Qt::UserRole,i);item->setToolTip(w.title);if(!w.ready)item->setForeground(QColor("#939393"));
            if(w.id==settings.value("lastId").toString())list->setCurrentItem(item);
        }
        if(!list->currentItem()&&list->count())list->setCurrentRow(0);
        count->setText(QString::number(wallpapers.size())+" wallpapers encontrados na biblioteca Steam");
        status->setText("Escolha um wallpaper e clique em Aplicar. Os arquivos continuam na biblioteca da Steam.");
        steamConfig.clear();for(const auto &lib:libs)if(QFile::exists(lib.root+"/steamapps/common/wallpaper_engine/config.json")){steamConfig=lib.root+"/steamapps/common/wallpaper_engine/config.json";break;}
    }
    void saveOptions() {
        settings.setValue("fps",fps->value());settings.setValue("mute",mute->isChecked());settings.setValue("follow",follow->isChecked());settings.setValue("login",login->isChecked());settings.sync();
        QString dir=QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)+"/autostart";QDir().mkpath(dir);
        QString path=dir+"/jocas-wallpapers.desktop";
        if(login->isChecked()) {
            QString exe=base+"/jocas-wallpapers";exe.replace("\\","\\\\").replace("\"","\\\"").replace("`","\\`").replace("$","\\$").replace("%","%%");
            QSaveFile f(path);if(f.open(QIODevice::WriteOnly)){f.write(("[Desktop Entry]\nType=Application\nName=JOCAS Wallpapers\nExec=\""+exe+"\" --restore\nIcon=preferences-desktop-wallpaper\nX-KDE-autostart-phase=2\n").toUtf8());f.commit();}
        } else QFile::remove(path);
    }
    void applySelected() {
        auto item=list->currentItem();if(!item)return;const auto&w=wallpapers[item->data(Qt::UserRole).toInt()];
        if(!w.ready){status->setText("Este wallpaper ainda não está pronto. Termine o download na Steam e clique em Atualizar lista.");return;}
        if(command("python3",{base+"/integration.py","--activate"}).isEmpty()){status->setText("Não foi possível ativar a integração do KDE. Confira a instalação.");return;}
        saveOptions();settings.setValue("lastId",w.id);settings.setValue("screen",screen->currentData());
        for(int i=1;i<screen->count();++i)if(screen->currentData()=="*"||screen->currentData()==screen->itemData(i))play(screen->itemData(i).toString(),w.path);
    }
    void play(QString output,QString path) {
        if(players.contains(output)){auto p=players.take(output);stopping.insert(p);p->terminate();if(!p->waitForFinished(2500)){p->kill();p->waitForFinished(1000);}}
        auto p=new QProcess(this);players[output]=p;applied[output]=path;
        auto logdir=QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation)+"/jocas-wallpapers";QDir().mkpath(logdir);
        p->setProcessChannelMode(QProcess::MergedChannels);p->setStandardOutputFile(logdir+"/"+output+".log",QIODevice::Truncate);
        QStringList args{"--screen-root",output,"--bg",path,"--fps",QString::number(fps->value()),"--scaling","fill"};
        if(mute->isChecked())args<<"--silent";
        for(const auto &lib:libs)if(QDir(lib.assets).exists()){args<<"--assets-dir"<<lib.assets;break;}
        connect(p,&QProcess::errorOccurred,this,[this,output](auto){status->setText("Não foi possível iniciar o wallpaper em "+output+". Verifique o diagnóstico em ~/.local/state/jocas-wallpapers.");});
        connect(p,&QProcess::finished,this,[this,p,output](int code,QProcess::ExitStatus){
            if(!stopping.remove(p)&&players.value(output)==p){players.remove(output);status->setText("A reprodução em "+output+" terminou (código "+QString::number(code)+"). Alguns efeitos do Wallpaper Engine ainda podem ser incompatíveis com o motor Linux.");}
            p->deleteLater();
        });
        p->start(base+"/engine/linux-wallpaperengine",args);
        if(!p->waitForStarted(4000)){players.remove(output);return;}
        settings.setValue("wallpapers/"+output,path);settings.sync();status->setText("Wallpaper iniciado em "+output+". Fechar esta janela mantém a animação e o ícone na bandeja.");
    }
    void stop(bool clear) {
        for(auto p:players){stopping.insert(p);p->terminate();if(!p->waitForFinished(1500)){p->kill();p->waitForFinished(1000);}}
        players.clear();applied.clear();if(clear){settings.remove("wallpapers");settings.sync();}
        status->setText("Animação parada. O KDE volta ao fundo estático salvo.");
    }
    void restoreDesktop() {
        stop(true);
        auto result=command("python3",{base+"/integration.py","--restore"});
        status->setText(result.isEmpty()?"Não foi possível restaurar automaticamente. Use as configurações de papel de parede do KDE.":QString::fromUtf8(result));
    }
    void restore() {
        settings.beginGroup("wallpapers");auto keys=settings.childKeys();QMap<QString,QString> saved;for(auto k:keys)saved[k]=settings.value(k).toString();settings.endGroup();
        if(!saved.isEmpty())command("python3",{base+"/integration.py","--activate"});
        for(auto it=saved.begin();it!=saved.end();++it){
            bool connected=false;for(int i=1;i<screen->count();++i)connected|=screen->itemData(i).toString()==it.key();
            if(connected&&QFile::exists(it.value()+"/project.json"))play(it.key(),it.value());
        }
    }
    void syncSteam() {
        if(steamConfig.isEmpty())return;
        auto d=readJson(steamConfig)["steamuser"].toObject()["general"].toObject()["wallpaperconfig"].toObject()["selectedwallpapers"].toObject();
        QString digest=QString::fromUtf8(QJsonDocument(d).toJson(QJsonDocument::Compact));if(digest==steamDigest)return;steamDigest=digest;
        for(auto it=d.begin();it!=d.end();++it){QString file=it.value().toObject()["file"].toString().replace('\\','/');if(file.isEmpty())continue;
            int index=it.key().mid(7).toInt()+1;if(index<1||index>=screen->count())continue;
            QString output=screen->itemData(index).toString();
            for(const auto&w:wallpapers)if(w.ready&&(file.contains("/431960/"+w.id+"/")||file.startsWith(w.path+"/"))&&applied.value(output)!=w.path){command("python3",{base+"/integration.py","--activate"});play(output,w.path);break;}
        }
    }
protected:
    void closeEvent(QCloseEvent *event)override{if(tray&&!quitting){hide();event->ignore();}else{stop(false);event->accept();}}
};

int main(int argc,char **argv) {
    QApplication app(argc,argv);app.setOrganizationName("JOCAS");app.setApplicationName("Wallpapers");app.setApplicationVersion("1.0.0");
    if(app.arguments().contains("--list")){QJsonArray out;for(const auto&w:scan(libraries()))out.append(QJsonObject{{"id",w.id},{"title",w.title},{"type",w.type},{"ready",w.ready}});QTextStream(stdout)<<QJsonDocument(out).toJson();return 0;}
    QString lockDir=QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation)+"/jocas-wallpapers";QDir().mkpath(lockDir);
    QLockFile lock(lockDir+"/instance.lock");
    if(!lock.tryLock(100)){QMessageBox::information(nullptr,"JOCAS Wallpapers","O JOCAS Wallpapers já está aberto. Clique no ícone dele na bandeja do KDE.");return 0;}
    Window window;
    if(app.arguments().contains("--restore")){window.restore();if(!QSystemTrayIcon::isSystemTrayAvailable())window.show();}else window.show();
    int apply=app.arguments().indexOf("--apply");
    if(apply>=0&&apply+1<app.arguments().size()) {
        QString id=app.arguments().at(apply+1);int output=app.arguments().indexOf("--screen");
        QString name=output>=0&&output+1<app.arguments().size()?app.arguments().at(output+1):"*";
        QTimer::singleShot(500,&window,[&window,id,name]{window.applyById(id,name);});
    }
    return app.exec();
}
