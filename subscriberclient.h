#ifndef SUBSCRIBERCLIENT_H
#define SUBSCRIBERCLIENT_H

#include "common.h"
#include <QThread>
#include <muduo/net/TcpClient.h>
#include <muduo/net/Channel.h>

using namespace muduo;
using namespace muduo::net;

class SubscriberClient : public QThread
{
public:
    Common* sharedData;
    explicit SubscriberClient(Common* sharedDataPtr, int port = 8008);
    ~SubscriberClient();
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                          muduo::net::Buffer* buf,
                          muduo::Timestamp receiveTime);
    void sendKeepAlive();
private:
    TcpClient* subClient; // `sub` names from `pub/sub` model.
    TcpConnectionPtr conn;
    int port;
    void run();
};

#endif // SUBSCRIBERCLIENT_H
