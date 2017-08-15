#include "testmain.h"
#include "queryhandler.h"
#include "updownhandler.h"
#include "subscriberclient.h"
#include <SimpleIni.h>
#include <cstdio>
#include <QDebug>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;

TestMain::TestMain(QObject *parent) : QObject(parent)
{
    conf.SetUnicode();
    conf.SetMultiKey(true);
    conf.LoadFile("ini.ini");
    sharedData.prgType = PG_INFO_SRV;
    const char* prgTypeVal = conf.GetValue("main", "program_type", "1");
    sharedData.prgType = (program_type)atoi(prgTypeVal);
    const char* querySrvPortVal = conf.GetValue("main", "query_srv_port", "8080");
    sharedData.querySrvPort = atoi(querySrvPortVal);
    const char* updownSrvPortVal = conf.GetValue("main", "updown_srv_port", "8061");
    sharedData.updownSrvPort = atoi(updownSrvPortVal);
    const char* subSrvPortVal = conf.GetValue("main", "sub_srv_port", "8008");
    sharedData.subSrvPort = atoi(subSrvPortVal);

    if (sharedData.prgType == PG_FILE_SRV) {
        sharedData.fileStorage.loadFSData();
        const char* fsIDVal = conf.GetValue("filesrv", "srv_id", "1");
        sharedData.srvID = atoi(fsIDVal);
    }

    if (sharedData.prgType == PG_INFO_SRV) {
        sharedData.fileStorage.loadISData();
        CSimpleIniA::TNamesDepend values;
        conf.GetAllValues("infosrv", "fs_ids", values);
        values.sort(CSimpleIniA::Entry::LoadOrder());

        CSimpleIniA::TNamesDepend::const_iterator iter;
        for (iter = values.begin(); iter != values.end(); ++iter) {
            sharedData.enabledSrvArr.push_back(atoi(iter->pItem));
        }
    }

#ifdef BLBLBLBLB
    qDebug() << sharedData.querySrvPort << sharedData.updownSrvPort << sharedData.subSrvPort;
    char buffer[61];
    snprintf(buffer, 6, "%d", (int)sharedData.prgType);
    conf.SetValue("main", "program_type", buffer);
    snprintf(buffer, 6, "%d", sharedData.querySrvPort);
    conf.SetValue("main", "query_srv_port", buffer);
    snprintf(buffer, 6, "%d", sharedData.updownSrvPort);
    conf.SetValue("main", "updown_srv_port", buffer);
    snprintf(buffer, 6, "%d", sharedData.subSrvPort);
    conf.SetValue("main", "sub_srv_port", buffer);
    conf.SaveFile("ini.ini");
#endif

}

TestMain::~TestMain()
{
    // prog will be terminaled by pressing Ctrl+C
    // so this func will not be triggered
}

void TestMain::main()
{
    LOG_INFO << "pid = " << getpid() << " Program Type: " << PG_TYPE_NAME[sharedData.prgType];

    Logger::setLogLevel(Logger::DEBUG);

    UpdownHandler updownSrv(&sharedData, sharedData.updownSrvPort);
    QueryHandler querySrv(&sharedData, sharedData.querySrvPort);
    SubscriberClient subSrv(&sharedData, sharedData.subSrvPort);

    sharedData.ptrUpdownHandler = &updownSrv;
    sharedData.ptrQueryHandler = &querySrv;
    sharedData.ptrSubscriberClient = &subSrv;

    updownSrv.start();
    querySrv.start(); // file srv disable query handling?
    if (sharedData.prgType == PG_FILE_SRV) {
        subSrv.start();
    }

    // wait till end.
    updownSrv.wait();

}
