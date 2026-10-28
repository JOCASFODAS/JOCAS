#include <QCoreApplication>
#include "../src/game_mode.hpp"
#include <iostream>
static QString current() { return QString::fromUtf8(pactl({"get-default-source"})); }
static void waitDefault(const QString &name) {
    for (int i=0;i<40;++i) { if(current()==name) return; QThread::msleep(10); }
    throw std::runtime_error("Default source did not settle");
}
static void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (argc != 4) return 2;
    QString a=argv[1], b=argv[2], c=argv[3];
    pactl({"set-default-source",a}); waitDefault(a);
    { GameMode mode(b); mode.enable(); mode.enable(); check(current()==b,"Game mode failed to select target"); mode.disable(); waitDefault(a); check(current()==a,"Previous microphone not restored"); }
    { GameMode mode(b); mode.enable(); pactl({"set-default-source",c}); waitDefault(c); mode.disable(); check(current()==c,"User's newer choice overwritten"); }
    pactl({"set-default-source",b}); waitDefault(b);
    { GameMode mode(b); mode.enable(); mode.disable(); check(current()==b,"Preselected target not preserved"); }
    pactl({"set-default-source",a}); waitDefault(a);
    { GameMode mode(b); mode.enable(); }
    waitDefault(a); check(current()==a,"Cleanup failed to restore original source");
    std::cout << "PASS: selection, repeated enable, stop, user choice, preselected target and destructor restore\n";
}
