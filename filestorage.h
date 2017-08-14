#ifndef FILESTORAGE_H
#define FILESTORAGE_H

#include <map>
#include <string>
#include <mutex>

enum program_type {
    PG_CLIENT,   // equals to 0
    PG_INFO_SRV, // master server
    PG_FILE_SRV  // chunk server
};

using namespace std;

class FileStorage
{
public:
    map<string, string> pathHashMap; //for FS, idxHashMap
    program_type storageOf;
    // if FILE_SRV
    int nextIndex = 0;
    map<int, int> idxPartMap;

    FileStorage();
    ~FileStorage();
    bool pushPathHash(string path_k, string hash_v);
    int pushFileRecord(string origFileHash, int filePartID);
    bool loadISData();
    bool loadFSData();
    bool saveISData();
    bool saveFSData();
private:
    mutex writeFileMutex;
    mutex insertDataMutex;

};

#endif // FILESTORAGE_H
