#include "../src/ai_panel.hpp"
#include <iostream>
static void check(bool ok,const char*s){if(!ok)throw std::runtime_error(s);}
static void click(AiPanel &p,const QString &name){for(auto*b:p.findChildren<QPushButton*>())if(b->text()==name){b->click();return;}throw std::runtime_error("Button missing");}
int main(int argc,char**argv){
 QApplication app(argc,argv);QTemporaryDir dir;QSettings s(dir.path()+"/preferences.ini",QSettings::IniFormat);
 {
  AiPanel p(s,false);click(p,"Ativar por API");
  for(auto *e:p.findChildren<QLineEdit*>())if(e->accessibleName()=="Chave da API OpenAI")e->setText("KEY_MUST_NOT_PERSIST_123");
  p.findChild<QCheckBox*>("aiConsent")->setChecked(true);
  for(auto*slider:p.findChildren<QSlider*>())if(slider->accessibleName()=="Velocidade da voz de IA")slider->setValue(115);
  p.save();s.sync();
 }
 check(s.value("ai/mode").toInt()==2,"API selection not saved");check(s.value("ai/speed").toInt()==115,"Voice controls not saved");
 for(const auto &k:s.allKeys())check(!s.value(k).toString().contains("KEY_MUST_NOT"),"API key persisted");
 {
  AiPanel p(s,false);check(!p.findChild<QCheckBox*>("aiConsent")->isChecked(),"Online consent restored across launch");
  for(auto*e:p.findChildren<QLineEdit*>())if(e->accessibleName()=="Chave da API OpenAI")check(e->text().isEmpty()&&e->echoMode()==QLineEdit::Password,"API key restored or exposed");
  bool rejected=false;try{p.captureSettings();}catch(const std::exception&){rejected=true;}check(rejected,"API starts without key/consent");
  click(p,"Efeitos atuais");check(!p.captureSettings().enabled,"Effects mode starts AI");
  int changes=0;p.onModeChanged=[&]{++changes;};p.setRunning(true);
  for(auto*b:p.findChildren<QPushButton*>())if(b->text()=="Usar SEM API"||b->text()=="Ativar por API"||b->text()=="Efeitos atuais")check(b->isEnabled(),"Mode buttons locked while audio is active");
  click(p,"Usar SEM API");check(changes==1,"Live mode request not delivered");
  click(p,"Usar SEM API");check(changes==1,"Same mode unnecessarily restarts audio");
  p.setRunning(false);click(p,"Ativar por API");
  for(auto*e:p.findChildren<QLineEdit*>())if(e->accessibleName()=="Chave da API OpenAI")check(e->isEnabled(),"API settings remain locked after pause");
 }
 if(app.arguments().contains("--prepare")) {
  AiPanel p(s,false);click(p,"Usar SEM API");bool busy=false,started=false;
  p.onBusy=[&](bool value){busy=value;started|=value;};click(p,"Preparar IA (primeiro uso)");
  QElapsedTimer timer;timer.start();
  while(busy && timer.elapsed()<180000){app.processEvents();QThread::msleep(10);}
  check(started&&!busy,"Prepare UI did not complete");
  check(QFileInfo::exists(AiPaths::root()+"/ready-1"),"Prepare UI did not save ready marker");
  check(p.captureSettings().enabled,"Prepared offline mode cannot start");
  std::cout<<"PASS prepare UI: isolated Python, locked dependencies and local model validation\n";
 }
 std::cout<<"PASS AI UI: preferences persist, key stays secret and session-only, consent resets, API prerequisites enforced\n";
}
