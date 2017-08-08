#include "updownhandler.h"

#include <string>
#include <boost/bind.hpp>

#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>

using namespace muduo;
using namespace muduo::net;

UpdownHandler::UpdownHandler(Common *sharedDataPtr, int port)
{
    this->sharedData = sharedDataPtr;
    this->port = port;
}

void UpdownHandler::onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected()) {
        //conn->shutdown();
    }
}

void UpdownHandler::onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp receiveTime, Common* sharedData)
{
    //Q_UNUSED(receiveTime)
    //char* buffer;buffer = buf->findCRLF(), buffer
    muduo::string buffer;
    if (buf->readableBytes() >= 4) {
        buffer.assign(buf->retrieveAsString(4));
        // first 4 bytes:
        //      Upper case means sender type and receiver type
        //      Lower case means action
        if (buffer.compare("FIka") == 0) {
            // filesrv -> infosrv keep-alive
            // read / update filesrv uptime info.
            // req: [*FIka*][FileSrvID]
            if (sharedData->prgType != PG_INFO_SRV) {
                conn->send("FUCK");
                conn->shutdown();
            }
            int fileSrvID = buf->readInt32();
            if (sharedData->srvStatus.count(fileSrvID) == 0) {
                LOG_DEBUG << "Received NEW keepalive from " << fileSrvID << " at " << receiveTime.toString();
                SrvStat yetAnotherSrvStat;
                yetAnotherSrvStat.lastKeepAlive = receiveTime;
                yetAnotherSrvStat.serverID = fileSrvID;
                yetAnotherSrvStat.serverAddr = conn->peerAddress();
                sharedData->srvStatus.insert(map<int, SrvStat>::value_type(fileSrvID, yetAnotherSrvStat));
            } else {
                LOG_DEBUG << "Received keepalive from " << fileSrvID << " at " << receiveTime.toString();
                SrvStat weGotaSrvStat = sharedData->srvStatus[fileSrvID];
                weGotaSrvStat.lastKeepAlive = receiveTime;
                sharedData->srvStatus[fileSrvID] = weGotaSrvStat;
            }
        } else if (buffer.compare("CIhq") == 0) {
            // Client -> infosrv Upload Hash Query, should response.
            // req: [*CIhq*][hash len][hash][file first 64 bytes][filesize(ssize_t?)]
            // rsp: [ICdc][addrcnt][addr][chunkcnt][chinkid1..2..][addr2]... (token?)
            if (sharedData->prgType != PG_INFO_SRV) {
                conn->send("FUCK");
                conn->shutdown();
            }

        } else if (buffer.compare("CFuc") == 0) {
            // Client -> filesrv Upload Chunk, should response.
            // req: [*CFuc*][chunk id][chunk len][chunk] (token?)
            // rsp: [FCui]["succ" / "fuck"]
        } else if (buffer.compare("CFdc") == 0) {
            // Client -> filesrv Download Chunk, should response.
            // req: [*CFdc*][chunk id][token for safty?]
            // rsp: [FCdc][chunkid][chunklen(ssize_t?)][chunk]
            if (sharedData->prgType != PG_FILE_SRV) {
                conn->send("FUCK");
                conn->shutdown();
            }
            // Query and response for trunk download.

        } else {
            LOG_INFO << "Weird Query: " << buffer;
        }
        LOG_INFO << buffer;
        // conn->send(":cry_on_big_news:\r\n");
        // conn->shutdown();
    }
    buf->retrieveAll(); // clean up unreaded.
}

void UpdownHandler::run()
{
    InetAddress updownListenAddr(port);
    EventLoop updownLoop;
    TcpServer updownSrv(&updownLoop, updownListenAddr, "UpdownSrv");
    updownSrv.setConnectionCallback(UpdownHandler::onConnection);
    updownSrv.setMessageCallback(boost::bind(&UpdownHandler::onMessage, _1, _2, _3, sharedData));
    updownSrv.setThreadNum(0);
    updownSrv.start();
    updownLoop.loop();
}
