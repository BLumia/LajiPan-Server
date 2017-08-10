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
    ~FileElement();
    int32_t id = -1;
    QString md5;
    QString ff16b;
    vector<int> chunkArray;
    int totalChunkCount;

    bool loadState(std::string md5);
    bool saveState();
    bool isBinaryFf16bEqual(string binFf16bBuffer);
private:
    bool inited = false;
};

#endif // FILEELEMENT_H
