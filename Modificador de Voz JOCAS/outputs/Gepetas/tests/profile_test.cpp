#define main jocas_application_main
#include "../src/main.cpp"
#undef main
#include <iostream>
static void check(bool ok, const char *msg) { if (!ok) throw std::runtime_error(msg); }
static QSlider *slider(Window &w, const QString &name) {
    for (auto *s : w.findChildren<QSlider *>()) if (s->accessibleName() == name) return s;
    throw std::runtime_error("Slider missing");
}
static void click(Window &w, const QString &name) {
    for (auto *b : w.findChildren<QPushButton *>()) if (b->text().startsWith(name)) { b->click(); return; }
    throw std::runtime_error("Button missing");
}
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QTemporaryDir temp;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temp.path());
    app.setOrganizationName("JOCASProfileTest"); app.setApplicationName("JOCASProfileTest");
    { QSettings s; s.setValue("pitch",-5); s.setValue("gate",-20); s.setValue("gain",200); s.setValue("preserve",false); }
    {
        Window w;
        check(slider(w,"Corte de ruído baixo")->value()==-60,"Legacy gate not corrected");
        check(slider(w,"Volume enviado")->value()==100,"Legacy gain not corrected");
        click(w,"Personalizada");
        slider(w,"Tom da voz")->setValue(-7);
        slider(w,"Intensidade do efeito extra")->setValue(73);
        slider(w,"Frequência do robô")->setValue(93);
        slider(w,"Distorção do rádio")->setValue(36);
        slider(w,"Graves")->setValue(4); slider(w,"Agudos / clareza")->setValue(2);
        for (auto *b:w.findChildren<QComboBox *>()) if(b->accessibleName()=="Efeito da voz personalizada") b->setCurrentIndex(1);
        click(w,"Natural");
        check(slider(w,"Tom da voz")->value()==0,"Natural did not reset pitch");
        click(w,"Personalizada");
        check(slider(w,"Tom da voz")->value()==-7,"Custom pitch lost after switching");
        check(slider(w,"Frequência do robô")->value()==93,"Custom frequency lost");
    }
    {
        Window w;
        check(slider(w,"Graves")->value()==4,"Custom EQ not saved");
        check(slider(w,"Intensidade do efeito extra")->value()==73,"Custom amount not saved");
        click(w,"Grave clara para Discord");
        check(slider(w,"Tom da voz")->value()==-5,"Deep clarity reset failed");
        click(w,"Personalizada");
        check(slider(w,"Tom da voz")->value()==-7,"Clarity reset overwrote custom voice");
    }
    QSettings s;
    check(s.value("beforeClarityUpdate/gate").toInt()==-20,"Legacy backup missing");
    check(s.value("custom/effect").toInt()==1,"Custom effect not persisted");
    std::cout << "Migration, independent custom profile, switching, restart and clarity reset OK\n";
}
