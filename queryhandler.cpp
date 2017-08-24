#include "queryhandler.h"
#include <muduo/base/Logging.h>
#include <muduo/net/http/HttpServer.h>
#include <muduo/net/http/HttpRequest.h>
#include <muduo/net/http/HttpResponse.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Timestamp.h>
#include <boost/bind.hpp>
#include <json/json.h>
#include <QFile>
#include <QFileInfo>

using namespace muduo;
using namespace muduo::net;

QueryHandler::QueryHandler(Common* sharedDataPtr, int port)
{
    this->sharedData = sharedDataPtr;
    this->port = port;
}

void QueryHandler::onRequest(Common* sharedData, const HttpRequest& req, HttpResponse* resp)
{
    LOG_INFO << "Headers " << req.methodString() << " " << req.path();

    if (req.path() == "/")
    {
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("text/html");
        resp->addHeader("Server", "LajiPan IS");
        muduo::string now = Timestamp::now().toString();
        resp->setBody("<html><head><title>Navigation Page</title></head>"
                      "<body><h1>Navigation Page</h1>"
                      //"Header Info: " + req.getHeader("NotExist") + "<br/>"
                      "<a href='./info'>InformationServer Status</a><br/>"
                      "<a href='./file'>File List | Chunk List</a><br/>"
                      "</body></html>");
    }
    else if (req.path().at(0) == '/' && req.path().at(1) == 'i')
    {
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("application/json");
        resp->addHeader("Server", "LajiPan IS");

        Json::Value root;

        int idx = 0;
        map<int, SrvStat>::iterator iter;
        for(iter=sharedData->srvStatus.begin(); iter!=sharedData->srvStatus.end(); iter++) {
            SrvStat gotaSrvStat = iter->second;
            root["fs_avaliable"][idx]["ID"] = gotaSrvStat.serverID;
            root["fs_avaliable"][idx]["addr"] = gotaSrvStat.serverAddr.toIpPort();
            root["fs_avaliable"][idx]["lastKeepAlive"] = gotaSrvStat.lastKeepAlive.toString();

            idx++;
        }

        root["fs_enabled"] = Json::arrayValue;
        vector<int>::iterator vecIter;
        for(vecIter=sharedData->enabledSrvArr.begin(); vecIter!=sharedData->enabledSrvArr.end(); vecIter++) {
            root["fs_enabled"].append(*vecIter);
        }

        Json::FastWriter writer;
        std::string rawData = writer.write(root);

        resp->setBody(rawData);
    }
    else if (req.path().at(0) == '/' && req.path().at(1) == 'f' && req.path().at(2) == 'i')
    {   // favicon ?????? nya nya nya ??????
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("application/json");
        resp->addHeader("Server", "LajiPan IS");

        Json::Value root;

        if (sharedData->prgType == PG_INFO_SRV) {
            root["filelist"] = Json::arrayValue;
            for (auto& kv : sharedData->fileStorage.pathHashMap) {
                root["filelist"].append(kv.first);
            }
        } else if (sharedData->prgType == PG_FILE_SRV) {
            root["chunklist"] = Json::arrayValue;
            Json::Value elem;
            for (auto& kv : sharedData->fileStorage.pathHashMap) {
                elem["id"] = kv.first;
                elem["ori_hash"] = kv.second;
                elem["part_id"] = sharedData->fileStorage.idxPartMap[stoi(kv.first)];
                root["chunklist"].append(elem);
            }
        } else {
            root["mamacat"] = "mia!";
        }


        Json::FastWriter writer;
        std::string rawData = writer.write(root);

        resp->setBody(rawData);
    }
    else if (req.path().at(0) == '/' && req.path().at(1) == 'd')
    {   // assume url: /download/{ChunkID}
        if (sharedData->prgType == PG_INFO_SRV) {
            resp->setStatusCode(HttpResponse::k400BadRequest);
            resp->setStatusMessage("Should be 403...");
            resp->setCloseConnection(true);
            return;
        }
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("application/octet-stream");
        resp->addHeader("Server", "LajiPan IS");

        // content len
        // blob
        int chunkID;
        char urlCopy[128];
        strcpy(urlCopy, req.path().c_str());
        sscanf(urlCopy + 10, "%d", &chunkID);
        QString fileFullPath = "FS_Index/" + QString::number(chunkID);
        QFile willDownload(fileFullPath);
        QFileInfo fileinfo(fileFullPath);
        qint64 fileLen = fileinfo.size();
        int chunkPartNum = sharedData->fileStorage.idxPartMap[chunkID];
        resp->addHeader("Content-Disposition", "inline; filename=\""+
                        QString::number(chunkPartNum).toStdString()+".dl\"");
        resp->addHeader("content-length", QString::number(fileLen).toStdString());

        if (!willDownload.open(QIODevice::ReadOnly)) {
            LOG_ERROR << "CAN NOT READ FILE";
            return;
        }
        QByteArray blob(fileLen, 0);
        blob = willDownload.readAll();

        resp->setBody(blob.toStdString());
    }
    else
    {
        resp->setStatusCode(HttpResponse::k404NotFound);
        resp->setStatusMessage("Not Found");
        resp->setCloseConnection(true);
    }
}

void QueryHandler::run()
{
    InetAddress queryListenAddr(port);
    EventLoop queryLoop;
    HttpServer querySrv(&queryLoop, queryListenAddr, "QuerySrv");
    querySrv.setHttpCallback(boost::bind(&QueryHandler::onRequest, sharedData, _1, _2));
    querySrv.setThreadNum(0);
    querySrv.start();
    queryLoop.loop();
}
