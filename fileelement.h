#ifndef FILEELEMENT_H
#define FILEELEMENT_H

#include <vector>
#include <string>
#include <QString>

using namespace std;

class FileElement
{
public:
    FileElement();
    int32_t id;
    QString md5;
    QString ff32b;
    vector<int> chunkArray;
    vector<int> notReadyChunkArray;

    bool loadState(std::string md5);
    bool saveState();
    bool isBinaryFf32bEqual(string binFf32bBuffer);
private:
    bool inited = false;
};

#endif // FILEELEMENT_H
