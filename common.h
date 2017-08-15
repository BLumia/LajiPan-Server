#ifndef COMMON_H
#define COMMON_H

#define MUDUO_STD_STRING
//#define BLBLBLBLB

#include <filestorage.h>
#include <map>
#include <vector>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Timestamp.h>

#define CHUNKSIZE_MB    64
#define CHUNKSIZE_B     67108861

using namespace std;
using namespace muduo;
using namespace muduo::net;

typedef struct server_status {
    int serverID;
    InetAddress serverAddr;
    Timestamp lastKeepAlive;
} SrvStat;

const string PG_TYPE_NAME[] {
    "PG_CLIENT",
    "PG_INFO_SRV",
    "PG_FILE_SRV"
};

class Common
{
public:
    program_type prgType;
    int querySrvPort, updownSrvPort, subSrvPort;
    // if prgType = PG_FILE_SRV
    int32_t srvID;
    // if prgType = PG_INFO_SRV
    FileStorage fileStorage;
    map<int, SrvStat> srvStatus;
    vector<int> enabledSrvArr;
    // for easily binding signals and solts
    void* ptrUpdownHandler = nullptr;
    void* ptrQueryHandler = nullptr;
    void* ptrSubscriberClient = nullptr;

    Common();
};

#endif // COMMON_H
