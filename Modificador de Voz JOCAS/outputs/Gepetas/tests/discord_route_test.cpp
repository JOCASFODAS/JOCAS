#include <QCoreApplication>
#include "../src/discord_route.hpp"
#include <iostream>
static void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    int source=1,other=1,moves=0,id=20;
    bool exists=true,blocked=false;
    auto query=[&](const QStringList &args)->QByteArray {
        if(args==QStringList{"-f","json","list","sources"})
            return exists ? R"([{"index":2,"name":"test_mic"}])" : "[]";
        if(args==QStringList{"-f","json","list","source-outputs"}) {
            QJsonArray streams;
            streams.append(QJsonObject{{"index",id},{"source",source},{"properties",QJsonObject{{"application.process.binary","Discord"}}}});
            streams.append(QJsonObject{{"index",21},{"source",other},{"properties",QJsonObject{{"application.process.binary","firefox"}}}});
            return QJsonDocument(streams).toJson();
        }
        check(args.size()==3&&args[0]=="move-source-output"&&args[1]==QString::number(id)&&args[2]=="test_mic","Unrelated stream modified");
        ++moves;
        if(blocked)throw std::runtime_error("Device busy");
        source=2;return {};
    };
    DiscordRoute route("test_mic",{"discord"},query);
    check(route.update().contains("conectado")&&source==2&&moves==1,"Successful move not verified");
    check(other==1,"Unrelated capture changed");
    route.update();check(moves==1,"Repeated route of connected stream");
    source=1;check(!route.update().contains("conectado")&&source==1&&moves==1,"Manual choice overwritten");
    check(route.update(true).contains("conectado")&&source==2&&moves==2,"Explicit retry failed");
    route.reset();route.update();source=1;route.update();check(moves==2,"Preselected stream manual choice overwritten");
    ++id;blocked=true;check(!route.update().contains("conectado")&&source==1,"Blocked move reported success");
    int attempts=moves;route.update();check(moves==attempts,"Blocked move retried continuously");
    blocked=false;++id;check(route.update().contains("conectado")&&source==2,"New Discord capture not detected");
    exists=false;check(route.update().contains("Inicie"),"Missing virtual input not handled");
    check(other==1,"Unrelated capture changed");
    std::cout<<"PASS: verified move, Discord-only filtering, existing/new streams, manual choice, blocked move, explicit retry and missing device (simulated server)\n";
}
