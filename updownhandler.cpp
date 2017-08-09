#include "updownhandler.h"
#include "fileelement.h"

#include <string>
#include <cstring>
#include <boost/bind.hpp>
#include <QDir>

#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>

using namespace muduo;
using namespace muduo::net;

UpdownHandler::UpdownHandler(Common *sharedDataPtr, int port)
{
    this->sharedData = sharedDataPtr;
    this->port = port;

    if (sharedDataPtr->prgType == PG_INFO_SRV) {
        if (!QDir("IS_Index").exists()) QDir().mkdir("IS_Index");
    }
    if (sharedDataPtr->prgType == PG_FILE_SRV) {
        if (!QDir("FS_Index").exists()) QDir().mkdir("FS_Index");
    }
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
    string buffer;
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
            // req: [*CIhq*][hash(32bytes)][file first 32 bytes][filesize(int64_t)]
            // rsp: [ICdc][rsplen]["404" / "403" / "200"] (if 404 then:)
            //      [addrcnt][addr][chunkcnt][chinkid1..2..][addr2]... (token?)
            if (sharedData->prgType != PG_INFO_SRV) {
                conn->send("FUCK");
                conn->shutdown();
            }
            string hashBuffer, ff32bBuffer;
            int64_t fileSize;

            hashBuffer = buf->retrieveAsString(32);
            ff32bBuffer = buf->retrieveAsString(32);
            fileSize = buf->readInt64();

            FileElement eleme;
            if (eleme.loadState(hashBuffer)) {
                if (eleme.isBinaryFf32bEqual(ff32bBuffer)) {
                    Buffer sendBuffer;
                    sendBuffer.append("ICdc", 4);
                    sendBuffer.appendInt32(3);
                    sendBuffer.append("200", 3);
                    conn->send(&sendBuffer);
                } else {
                    Buffer sendBuffer;
                    sendBuffer.append("ICdc", 4);
                    sendBuffer.appendInt32(3);
                    sendBuffer.append("403", 3);
                    conn->send(&sendBuffer);
                }
            } else {
                // ?
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

        } else if (buffer.compare("PING") == 0) {
            // just for test, rsp: "PONG"
            conn->send("PONG");
            conn->shutdown();

        } else {
            LOG_INFO << "Weird Query: " << buffer;
        }
        LOG_INFO << buffer;
        // conn->send(":cry_on_big_news:\r\n");
        // conn->shutdown();
    }
    // dont do this in final version!!!!!!
    buf->retrieveAll(); // clean up unreaded. this may cause problem if data not yet fully received.
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
