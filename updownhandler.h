#ifndef UPDOWNHANDLER_H
#define UPDOWNHANDLER_H

#include "common.h"
#include <QThread>
#include <muduo/net/TcpServer.h>

using namespace muduo;
using namespace muduo::net;

class UpdownHandler : public QThread
{
    Q_OBJECT
public:
    Common* sharedData;
    explicit UpdownHandler(Common* sharedDataPtr, int port = 8061);
    static void onConnection(const TcpConnectionPtr& conn);
    static void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime, Common *sharedData);
private:
    int port = 8061;
    void run();
signals:
    void sendFIruData(string hashBuffer, int32_t chunkID);
};

#endif // UPDOWNHANDLER_H
