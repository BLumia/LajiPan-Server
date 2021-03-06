#include "subscriberclient.h"
#include <muduo/net/EventLoop.h>
#include <boost/bind.hpp>
#include <muduo/net/Socket.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/Logging.h>
#include <QtConcurrent>
#include <QDebug>

using namespace muduo;
using namespace muduo::net;

SubscriberClient::SubscriberClient(Common *sharedDataPtr, int port)
{
    this->sharedData = sharedDataPtr;
    this->port = port;
}

SubscriberClient::~SubscriberClient()
{
    delete subClient;
}

void SubscriberClient::onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected())
    {
        this->conn = conn;
        this->conn->setTcpNoDelay(true);
        // KeepAlive use QtConcurrent?
        QtConcurrent::run(this, &SubscriberClient::sendKeepAlive);
        qDebug() << "conn succ";
    }
    else
    {
        this->conn.reset();
        qDebug() << "conn fuck";
    }
}

void SubscriberClient::onMessage(const TcpConnectionPtr &conn,
                                 Buffer *buf,
                                 Timestamp receiveTime)
{
    Q_UNUSED(buf)
    Q_UNUSED(receiveTime)
    Q_UNUSED(conn)
    // conn->send(":what:\r\n");
}

void SubscriberClient::sendKeepAlive()
{
    for(;;) {
        sleep(5);
        Buffer sendBuffer;
        sendBuffer.append("FIka");
        sendBuffer.appendInt32(sharedData->srvID);
        sendBuffer.appendInt32(sharedData->updownSrvPort);
        //conn->send("FIka");
        //int32_t asd = sockets::hostToNetwork32(sharedData->srvID);
        conn->send(&sendBuffer);
        //conn->send(&asd, sizeof(asd));
        //conn->send(&(sharedData->srvID), sizeof(sharedData->srvID));
    }

}

void SubscriberClient::sendFIruData(string hashBuffer, int32_t chunkID)
{
    if (sharedData->prgType != PG_FILE_SRV) {
        LOG_FATAL << "sendFIruData(): NOT A FILE SRV!!!!!!!!!!!!";
        return;
    }
    Buffer sendBuffer;
    sendBuffer.append("FIru");
    sendBuffer.append(hashBuffer.c_str(), 32);
    sendBuffer.appendInt32(chunkID);
    sendBuffer.appendInt32(sharedData->srvID);
    conn->send(&sendBuffer);
    LOG_INFO << "Sent FIru:" << hashBuffer << " " << chunkID;
}

void SubscriberClient::run()
{
    EventLoop fsLoop;
    InetAddress fsListenAddr(port);
    subClient = new TcpClient(&fsLoop, fsListenAddr, "SubSrv");
    subClient->setConnectionCallback(
                boost::bind(&SubscriberClient::onConnection, this, _1));
    subClient->setMessageCallback(
                boost::bind(&SubscriberClient::onMessage, this, _1, _2, _3));
    subClient->connect();
    fsLoop.loop();
}
