#ifndef QUERYHANDLER_H
#define QUERYHANDLER_H

#include "common.h"
#include <QThread>
#include <muduo/net/http/HttpServer.h>
#include <muduo/net/http/HttpRequest.h>
#include <muduo/net/http/HttpResponse.h>

using namespace muduo;
using namespace muduo::net;

class QueryHandler : public QThread
{
public:
    Common* sharedData;
    explicit QueryHandler(Common* sharedDataPtr, int port = 8080);
    static void onRequest(Common *sharedData, const HttpRequest& req, HttpResponse* resp);
private:
    int port = 8080;
    void run();
};

#endif // QUERYHANDLER_H
