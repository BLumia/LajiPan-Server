#include "common.h"

Common::Common()
{

}

int Common::getFSIDbyAddr(const map<int, SrvStat> &srvStatus, InetAddress addr)
{
    for (const pair<int, SrvStat> &onePair : srvStatus) {
        const InetAddress &oneAddr = onePair.second.serverAddr;
        if (oneAddr.toIp() == addr.toIp() && oneAddr.toPort() == addr.toPort())
            return onePair.first;
    }
    return -1; //not found
}
