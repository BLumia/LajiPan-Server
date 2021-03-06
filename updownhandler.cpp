#include "updownhandler.h"
#include "fileelement.h"
#include "subscriberclient.h"

#include <string>
#include <cstring>
#include <boost/bind.hpp>
#include <QDir>
#include <arpa/inet.h>

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
    if (!QDir("Backup").exists()) QDir().mkdir("Backup");

    connect(this, SIGNAL(sendFIruData(string, int32_t)),
            (SubscriberClient*)(sharedDataPtr->ptrSubscriberClient), SLOT(sendFIruData(string, int32_t)));
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
        const char* headerPointer = buf->peek();
        char f4b[5]; f4b[4] = '\0';
        memcpy(f4b, headerPointer, 4);
        buffer = f4b;
        // first 4 bytes:
        //      Upper case means sender type and receiver type
        //      Lower case means action
        if (buffer.compare("FIka") == 0) {
            // filesrv -> infosrv keep-alive
            // read / update filesrv uptime info.
            // req: [*FIka*][FileSrvID][FileSrvUpdownPort]
            if (buf->readableBytes() < (4 + 4 + 4)) return; // wait for next read.
            else buf->retrieve(4); // retrieve f4b header marker.

            if (sharedData->prgType != PG_INFO_SRV) {
                conn->send("FUCK");
                conn->shutdown();
            }
            int fileSrvID = buf->readInt32();
            int fileSrvPort = buf->readInt32();
            if (sharedData->srvStatus.count(fileSrvID) == 0) {
                LOG_DEBUG << "Received NEW keepalive from " << fileSrvID << " at " << receiveTime.toString();
                SrvStat yetAnotherSrvStat;
                yetAnotherSrvStat.lastKeepAlive = receiveTime;
                yetAnotherSrvStat.serverID = fileSrvID;
                yetAnotherSrvStat.serverAddr = InetAddress(conn->peerAddress().toIp(), fileSrvPort);
                sharedData->srvStatus.insert(map<int, SrvStat>::value_type(fileSrvID, yetAnotherSrvStat));
            } else {
                LOG_TRACE << "Received keepalive from " << fileSrvID << " at " << receiveTime.toString();
                SrvStat weGotaSrvStat = sharedData->srvStatus[fileSrvID];
                weGotaSrvStat.lastKeepAlive = receiveTime;
                sharedData->srvStatus[fileSrvID] = weGotaSrvStat;
            }
        } else if (buffer.compare("CIhq") == 0) {
            // Client -> infosrv Upload Hash Query, should response.
            // req: [*CIhq*][hash(32bytes)][file first 16 bytes(hex, so 32bytes)]
            //      [filesize(int64_t)][256byte filename]
            // rsp: [ICuc][rsplen]["404" / "403" / "200"] (if 404 then:)
            //      [chunkcnt][addrcnt][addr1..2..]
            if (buf->readableBytes() < (4 + 32 + 32 + 8 + 256)) return; // wait for next read.
            else buf->retrieve(4); // retrieve f4b header marker.

            if (sharedData->prgType != PG_INFO_SRV) {
                conn->send("FUCK");
                conn->shutdown();
            }
            string hashBuffer, ff16bBuffer, fileName;
            int64_t fileSize;

            hashBuffer = buf->retrieveAsString(32);
            ff16bBuffer = buf->retrieveAsString(32);
            fileSize = buf->readInt64();
            fileName = buf->retrieveAsString(256);

            FileElement eleme;
            if (eleme.loadState(hashBuffer)) {
                if (eleme.ff16b.trimmed().compare(QString::fromStdString(ff16bBuffer).trimmed()) == 0) {
                    Buffer sendBuffer;
                    sendBuffer.append("ICuc", 4);
                    sendBuffer.appendInt32(3);
                    sendBuffer.append("200", 3);
                    conn->send(&sendBuffer);

                    // we already got this file so just add the file record to .storage indexing file.
                    // dont upload at all.
                    QString cleanedFileName = QString::fromStdString(fileName);
                    cleanedFileName = cleanedFileName.simplified();
                    // save file to fileStorage indexing db
                    sharedData->fileStorage.pushPathHash(cleanedFileName.toStdString(), hashBuffer);
                    sharedData->fileStorage.saveISData();
                } else {
                    // server had this file, but user didnt. user pretend to upload
                    // inorder to dl. dont allowed this.
                    Buffer sendBuffer;
                    sendBuffer.append("ICuc", 4);
                    sendBuffer.appendInt32(3);
                    sendBuffer.append("403", 3);
                    conn->send(&sendBuffer);
                    LOG_WARN << eleme.ff16b.toStdString() << " | " << ff16bBuffer;
                }
            } else {
                // eleme init
                eleme.md5 = QString::fromStdString(hashBuffer);
                eleme.ff16b = QString::fromStdString(ff16bBuffer);
                QString cleanedFileName = QString::fromStdString(fileName);
                cleanedFileName = cleanedFileName.simplified();
                // save file to fileStorage indexing db
                sharedData->fileStorage.pushPathHash(cleanedFileName.toStdString(), hashBuffer);
                sharedData->fileStorage.saveISData();
                // rsp
                int32_t chunkCnt = fileSize / CHUNKSIZE_B;
                if (fileSize % CHUNKSIZE_B != 0) chunkCnt++;
                int32_t addrCnt = sharedData->enabledSrvArr.size();
                int32_t sendSize;
                sendSize = 3 + 2 * sizeof(int32_t) + addrCnt * 2 * sizeof(uint32_t) + 256;
                eleme.totalChunkCount = chunkCnt;
                Buffer sendBuffer;
                sendBuffer.append("ICuc", 4); // send size not included this.
                sendBuffer.appendInt32(sendSize); // send size not included this.
                sendBuffer.append("404", 3);
                sendBuffer.appendInt32(chunkCnt);
                sendBuffer.appendInt32(addrCnt);
                for(int i = 0; i< addrCnt; i++) {
                    int srvIdx = sharedData->enabledSrvArr[i];
                    uint32_t addr = sharedData->srvStatus[srvIdx].serverAddr.ipNetEndian();
                    int32_t port = sharedData->srvStatus[srvIdx].serverAddr.toPort();
                    //sendBuffer.append((char*)&addr, sizeof(addr));
                    sendBuffer.appendInt32(addr);
                    sendBuffer.appendInt32(port);
                }
                conn->send(&sendBuffer);
            }

        } else if (buffer.compare("CIsr") == 0) {
            // Client -> infosrv Servers Stats Request, should response.
            // req: [*CIsr*]
            // rsp: [ICsr][rsplen][addrcnt][addr1..2..]
            buf->retrieve(4); // retrieve f4b header marker.

            if (sharedData->prgType != PG_INFO_SRV) {
                conn->send("FUCK");
                conn->shutdown();
            }

            Buffer sendBuffer;

            int32_t addrCnt = sharedData->enabledSrvArr.size();
            int32_t sendSize;
            sendSize = sizeof(int32_t) + addrCnt * 3 * sizeof(uint32_t);

            sendBuffer.append("ICsr", 4); // send size not included this.
            sendBuffer.appendInt32(sendSize); // send size not included this.
            sendBuffer.appendInt32(addrCnt);

            for(int i = 0; i< addrCnt; i++) {
                int srvIdx = sharedData->enabledSrvArr[i];
                uint32_t addr = sharedData->srvStatus[srvIdx].serverAddr.ipNetEndian();
                int32_t port = sharedData->srvStatus[srvIdx].serverAddr.toPort();
                //sendBuffer.append((char*)&addr, sizeof(addr));
                sendBuffer.appendInt32(srvIdx);
                sendBuffer.appendInt32(addr);
                // FIXME: maybe should provide a Query(HTTP) Port?
                sendBuffer.appendInt32(port);
            }

            conn->send(&sendBuffer);

        } else if (buffer.compare("CIbr") == 0) {
            // Client -> infosrv Backup Request, should response.
            // req: [*CIbr*][uint32 addr][int32 port][int32 chunkid]
            // rsp: [ICbi][uint32 addr][int32 port][int32 FSID*1000+chunkid]
            if (buf->readableBytes() < (4 + 4 + 4 + 4)) return; // wait for next read.
            else buf->retrieve(4); // retrieve f4b header marker.

            if (sharedData->prgType != PG_INFO_SRV) {
                conn->send("FUCK");
                conn->shutdown();
            }

            uint32_t fsaddr = buf->readInt32();
            int32_t fsport = buf->readInt32();
            int32_t chunkid = buf->readInt32();
            char str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(fsaddr), str, INET_ADDRSTRLEN);
            string a(str);
            InetAddress fsia(a, fsport);
            int fsid = Common::getFSIDbyAddr(sharedData->srvStatus, fsia);
            int retfsidnidx = fsid * 1000 + chunkid;

            // mark srv down?
            LOG_DEBUG << "Backup Req: " << retfsidnidx << " aka " << a << ":" << fsport;

            Buffer sendBuffer;

            sendBuffer.append("ICbi", 4);
            sendBuffer.appendInt32(0);
            sendBuffer.appendInt32(0);
            sendBuffer.appendInt32(-1);

            conn->send(&sendBuffer);

        } else if (buffer.compare("CFuc") == 0) {
            // Client -> filesrv Upload Chunk, should response.
            // req: [*CFuc*][chunk len(int64)][file hash][file part id(for client)][chunk] (token?)
            // rsp: [FCui]["succ" / "fuck"]
            if (buf->readableBytes() < 12) return; // should also have chunk len.
            buf->retrieve(4);

            int64_t chunkSize = buf->peekInt64();
            if (buf->readableBytes() < (0 + 8 + 32 + 4 + chunkSize)) {
                buf->prepend("CFuc", 4);
                return;
            } else {
                buf->retrieve(8); // chunkSize
            }

            if (sharedData->prgType != PG_FILE_SRV) {
                conn->send("FUCK");
                conn->shutdown();
                return;
            }

            string hashBuffer = buf->retrieveAsString(32);
            int filePartId = buf->readInt32();
            string fileStreamBuffer = buf->retrieveAsString(chunkSize);

            QByteArray fileBinary(fileStreamBuffer.c_str(), chunkSize);
            int fileIdx = sharedData->fileStorage.pushFileRecord(hashBuffer, filePartId);
            sharedData->fileStorage.saveFSData();
            QFile file("FS_Index/" + QString::number(fileIdx));
            file.open(QIODevice::WriteOnly);
            file.write(fileBinary);
            file.close();

            Buffer sendBuffer;
            sendBuffer.append("FCuisucc", 8);
            conn->send(&sendBuffer);

            SubscriberClient* scptr = (SubscriberClient*)sharedData->ptrSubscriberClient;
            scptr->sendFIruData(hashBuffer, fileIdx);

        } else if (buffer.compare("FIru") == 0) {
            // filesrv -> infosrv Report Upload
            // req: [*FIru*][file hash][chunk id][FileSrv ID] (and token?)
            // rsp: tan 90°
            if (sharedData->prgType != PG_INFO_SRV) {
                conn->send("FUCK");
                conn->shutdown();
            }

            if (buf->readableBytes() < (4 + 32 + 4 + 4)) return; // wait for next read.
            else buf->retrieve(4); // retrieve f4b header marker.

            string hashBuffer = buf->retrieveAsString(32);
            int32_t chunkPartID = buf->readInt32();
            int32_t fileSrvID = buf->readInt32();

            LOG_INFO << "------hash: " << hashBuffer << "  --part:  " << chunkPartID << " --FSID: " << fileSrvID;
            FileElement eleme;
            if (eleme.loadState(hashBuffer)) {
                eleme.chunkArray.push_back(chunkPartID + fileSrvID * 1000);
                eleme.saveState();
            } else {
                LOG_WARN << "Trying to save or update not exist file state";
            }

        } else if (buffer.compare("CFdb") == 0) {
            // Client -> filesrv Download Backup chunk, should response.
            // req: [*CFdb*][chunk id][token for safty?]
            // rsp: [FCdb][chunkpartid][chunklen(int64_t)][chunk]
            if (sharedData->prgType != PG_FILE_SRV) {
                conn->send("FUCK");
                conn->shutdown();
                return;
            }
            if (buf->readableBytes() < (4 + 4 + 0)) return; // wait for next read.
            else buf->retrieve(4);
            int32_t chunkID = buf->readInt32();
            // Query and response for trunk download.
            QString fileFullPath = "Backup/" + QString::number(chunkID);
            QFile willDownload(fileFullPath);
            QFileInfo fileinfo(fileFullPath);
            qint64 fileLen = fileinfo.size();
            int chunkPartNum = 1; // FIXME: always 1

            if (!willDownload.open(QIODevice::ReadOnly)) {
                LOG_ERROR << "CFdb: CAN NOT READ FILE";
                conn->shutdown();
                return;
            }
            QByteArray blob(fileLen, 0);
            blob = willDownload.readAll();

            Buffer sendBuffer;
            sendBuffer.append("FCdb", 4);
            sendBuffer.appendInt32(chunkPartNum);
            sendBuffer.appendInt64(fileLen);
            sendBuffer.append(blob.data(), fileLen);

            conn->send(&sendBuffer);

        } else if (buffer.compare("CFdc") == 0) {
            // Client -> filesrv Download Chunk, should response.
            // req: [*CFdc*][chunk id][token for safty?]
            // rsp: [FCdc][chunkpartid][chunklen(int64_t)][chunk]
            if (sharedData->prgType != PG_FILE_SRV) {
                conn->send("FUCK");
                conn->shutdown();
                return;
            }
            if (buf->readableBytes() < (4 + 4 + 0)) return; // wait for next read.
            else buf->retrieve(4); // retrieve f4b header marker.
            int32_t chunkID = buf->readInt32();
            // Query and response for trunk download.
            QString fileFullPath = "FS_Index/" + QString::number(chunkID);
            QFile willDownload(fileFullPath);
            QFileInfo fileinfo(fileFullPath);
            qint64 fileLen = fileinfo.size();
            int chunkPartNum = sharedData->fileStorage.idxPartMap[chunkID];

            if (!willDownload.open(QIODevice::ReadOnly)) {
                LOG_ERROR << "CFdc: CAN NOT READ FILE";
                conn->shutdown();
                return;
            }
            QByteArray blob(fileLen, 0);
            blob = willDownload.readAll();

            Buffer sendBuffer;
            sendBuffer.append("FCdc", 4);
            sendBuffer.appendInt32(chunkPartNum);
            sendBuffer.appendInt64(fileLen);
            sendBuffer.append(blob.data(), fileLen);

            conn->send(&sendBuffer);

        } else if (buffer.compare("CIfq") == 0) {
            // Client -> Infosrv File Query
            // req: [*CIfq*][256byte filename]
            // rsp: [ICdc][chunkCnt][chunk1..2..3..]
            if (sharedData->prgType != PG_INFO_SRV) {
                conn->send("FUCK");
                conn->shutdown();
                return;
            }
            if (buf->readableBytes() < 4 + 256) return; // should also have chunk len.
            buf->retrieve(4);
            string fileName = buf->retrieveAsString(256);
            string cleanedFileName(fileName.c_str());
            map<string, string>* phmPtr = &(sharedData->fileStorage.pathHashMap);
            vector<int> resultArr;
            if (phmPtr->find(cleanedFileName) != phmPtr->end()) {
                string fileHash = (*phmPtr->find(cleanedFileName)).second;
                LOG_INFO << "Found hash: " + fileHash;
                FileElement eleme;
                eleme.loadState(fileHash);
                if (eleme.chunkArray.size() == eleme.totalChunkCount) {
                    resultArr = eleme.chunkArray;
                }
            }
            // ensure data are inside `resultArr`.
            Buffer sendBuffer;
            int32_t size = resultArr.size();
            sendBuffer.append("ICdc", 4);
            sendBuffer.appendInt32(size);
            for (int32_t chunkid : resultArr) {
                sendBuffer.appendInt32(chunkid);
            }
            conn->send(&sendBuffer);

        } else if (buffer.compare("PING") == 0) {
            // just for test, rsp: "PONG"
            buf->retrieve(4);
            conn->send("PONG");
            conn->shutdown();
        } else {
            LOG_INFO << "Weird Query: " << buffer;
        }
        LOG_TRACE << buffer << " proccessed.";
        // conn->send(":cry_on_big_news:\r\n");
        // conn->shutdown();
    }
    // dont do this in final version!!!!!!
    // buf->retrieveAll(); clean up unreaded. this may cause problem if data not yet fully received.
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
