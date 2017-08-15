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
    Q_OBJECT
public:
    Common* sharedData;
    explicit SubscriberClient(Common* sharedDataPtr, int port = 8008);
    ~SubscriberClient();
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                          muduo::net::Buffer* buf,
                          muduo::Timestamp receiveTime);
    void sendKeepAlive();
public slots:
    void sendFIruData(string hashBuffer, int32_t chunkID);
private:
    TcpClient* subClient; // `sub` names from `pub/sub` model.
    TcpConnectionPtr conn;
    int port;
    void run();
};

#endif // SUBSCRIBERCLIENT_H
