#include "queryhandler.h"
#include <muduo/base/Logging.h>
#include <muduo/net/http/HttpServer.h>
#include <muduo/net/http/HttpRequest.h>
#include <muduo/net/http/HttpResponse.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Timestamp.h>
#include <boost/bind.hpp>
#include <json/json.h>

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
    else if (req.path().at(0) == '/' && req.path().at(1) == 'f')
    {
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
            // TODO: display chunk state
        } else {
            root["mamacat"] = "mia!";
        }


        Json::FastWriter writer;
        std::string rawData = writer.write(root);

        resp->setBody(rawData);
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
