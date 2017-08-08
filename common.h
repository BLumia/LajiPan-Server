#ifndef COMMON_H
#define COMMON_H

//#define BLBLBLBLB

#include <map>
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

class Common
{
public:
    map<int, SrvStat> srvStatus;
    program_type prgType;
    int querySrvPort, updownSrvPort, subSrvPort;
    Common();
};

#endif // COMMON_H
