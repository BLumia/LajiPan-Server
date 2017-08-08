#ifndef COMMON_H
#define COMMON_H

#define MUDUO_STD_STRING
//#define BLBLBLBLB

#include <map>
#include <vector>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Timestamp.h>

using namespace std;
using namespace muduo;
using namespace muduo::net;

typedef struct server_status {
    int serverID;
    InetAddress serverAddr;
    Timestamp lastKeepAlive;
} SrvStat;

enum program_type {
    PG_CLIENT,   // equals to 0
    PG_INFO_SRV, // master server
    PG_FILE_SRV  // chunk server
};

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
    map<int, SrvStat> srvStatus;
    vector<int> enabledSrvArr;

    Common();
};

#endif // COMMON_H
