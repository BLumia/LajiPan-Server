#include "subscriberclient.h"
#include <muduo/net/EventLoop.h>
#include <boost/bind.hpp>
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
        this->conn->send("fuck?");
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
    conn->send(":what:\r\n");
}

void SubscriberClient::sendKeepAlive()
{
    for(;;) {
        sleep(5);
        conn->send("FIka");
    }

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
