#define JOCAS_AUDIO_CLIENT_NAME "JOCAS Mode Switch Synthetic Test"
#define JOCAS_VIRTUAL_NAMESPACE "jocas_switch_test"
#define main jocas_main
#include "../src/main.cpp"
#undef main
#include <iostream>
static void check(bool ok,const char*s){if(!ok)throw std::runtime_error(s);}
int main(int argc,char**argv){
 QApplication app(argc,argv);if(argc!=3)return 2;
 app.setApplicationName("JocasSwitchTest");app.setOrganizationName("JocasSwitchTest");
 Window w;w.show();QString sourceId;
 auto button=[&](QString s){for(auto*b:w.findChildren<QPushButton*>())if(b->text().contains(s))return b;throw std::runtime_error("Button missing");};
 auto source=[&](){for(auto v:QJsonDocument::fromJson(pactl({"-f","json","list","sources"})).array()){auto o=v.toObject();if(o["name"].toString()=="jocas_switch_test_mic")return QString::number(o["index"].toInt());}return QString();};
 QTimer::singleShot(300,[&]{
  for(auto*b:w.findChildren<QComboBox*>())if(b->accessibleName()=="Microfone de entrada"||b->accessibleName()=="Fones para escuta"){
   QString id=b->accessibleName()=="Microfone de entrada"?argv[1]:argv[2];int i=b->findData(id);check(i>=0,"Fixture missing");b->setCurrentIndex(i);
  }
  w.findChild<QCheckBox*>("discordAuto")->setChecked(false);w.findChild<QCheckBox*>("gameMode")->setChecked(false);
  button("Efeitos atuais")->click();button("Grave clara")->click();button("Iniciar microfone")->click();
 });
 QTimer::singleShot(1800,[&]{sourceId=source();check(!sourceId.isEmpty(),"Virtual microphone missing");});
 QTimer::singleShot(2200,[&]{check(button("Usar SEM API")->isEnabled(),"Offline button disabled while live");button("Usar SEM API")->click();std::cerr<<"MODE CLICKED\n";});
 QTimer::singleShot(3400,[&]{
  check(button("Iniciar microfone")->isEnabled(),"Start not restored after pause");check(source()==sourceId,"Virtual mic removed or replaced during pause");
  auto *p=w.findChild<QGroupBox*>("aiPanel");bool unlocked=false;for(auto*c:p->findChildren<QComboBox*>())if(c->accessibleName()=="Voz local de IA")unlocked=c->isEnabled();check(unlocked,"Offline settings remain disabled");
  button("Ativar por API")->click();bool api=false;for(auto*e:p->findChildren<QLineEdit*>())if(e->accessibleName()=="Chave da API OpenAI")api=e->isEnabled();check(api,"API field cannot be edited");
 });
 QTimer::singleShot(4300,[&]{
  QTimer::singleShot(80,[]{for(auto*w:QApplication::topLevelWidgets())if(auto*b=qobject_cast<QMessageBox*>(w))b->accept();});
  button("Iniciar microfone")->click();check(source()==sourceId,"Failed validation removed virtual mic");std::cerr<<"VALIDATION RETAINED INPUT\n";
 });
 QTimer::singleShot(6000,[&]{button("Efeitos atuais")->click();button("Iniciar microfone")->click();});
 QTimer::singleShot(7500,[&]{check(source()==sourceId,"Resume replaced virtual microphone");std::cerr<<"RESUMED SAME INPUT\n";});
 QTimer::singleShot(8500,[&]{button("Usar SEM API")->click();});
 QTimer::singleShot(10000,[&]{check(source()==sourceId,"Second switch removed microphone");w.close();app.quit();});
 int result=app.exec();std::cout<<"PASS: enabled live mode buttons, editable paused settings, validation preserves input, same-device resume and cleanup\n";return result;
}
