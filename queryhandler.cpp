#include "queryhandler.h"
#include <muduo/base/Logging.h>
#include <muduo/net/http/HttpServer.h>
#include <muduo/net/http/HttpRequest.h>
#include <muduo/net/http/HttpResponse.h>
#include <muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;

QueryHandler::QueryHandler(Common* sharedDataPtr, int port)
{
    this->sharedData = sharedDataPtr;
    this->port = port;
}

void QueryHandler::onRequest(const HttpRequest& req, HttpResponse* resp)
{
    LOG_INFO << "Headers " << req.methodString() << " " << req.path();

    if (req.path() == "/")
    {
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("text/html");
        resp->addHeader("Server", "Muduo");
        muduo::string now = Timestamp::now().toFormattedString();
        resp->setBody("<html><head><title>This is title</title></head>"
                      "<body><h1>Hello</h1>Header Info: " + req.getHeader("NotExist") +
                      "</body></html>");
    }
    else if (req.path().at(1) == 'c')
    {
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("text/plain");
        resp->addHeader("Server", "Muduo");
        resp->setBody("<h1>C!</h1>\n");
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
    querySrv.setHttpCallback(QueryHandler::onRequest);
    querySrv.setThreadNum(0);
    querySrv.start();
    queryLoop.loop();
}
