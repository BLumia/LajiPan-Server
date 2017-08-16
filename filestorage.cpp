#include "filestorage.h"

#include <QString>
#include <QFileInfo>
#include <QTextStream>
#include <muduo/base/Logging.h>

FileStorage::FileStorage()
{

}

FileStorage::~FileStorage()
{
    if (this->storageOf == PG_INFO_SRV) saveISData();
    else if (this->storageOf == PG_FILE_SRV) saveFSData();
}

bool FileStorage::pushPathHash(string path_k, string hash_v)
{
    string cleanedPath(path_k.c_str());
    //path_k.erase(path_k.find_last_not_of(" \n\v\f\r\t")+1);
    hash_v.erase(hash_v.find_last_not_of(" \n\v\f\r\t")+1);
    pathHashMap.insert(pair<string, string>(cleanedPath , hash_v));
    return true;
}

int FileStorage::pushFileRecord(string origFileHash, int filePartID)
{
    lock_guard<mutex> guardian(insertDataMutex); // protect for index.

    pathHashMap.insert(pair<string, string>(std::to_string(this->nextIndex) , origFileHash));
    idxPartMap.insert(pair<int, int>(this->nextIndex, filePartID) );
    this->nextIndex++;
    return (this->nextIndex - 1);
}

bool FileStorage::loadISData()
{
    LOG_INFO << "loading IS storage data...";

    this->storageOf = PG_INFO_SRV;
    QString path = "IS_Index/.storage";
    QFileInfo checkFileInfo(path);
    if (checkFileInfo.exists() && checkFileInfo.isFile()) {
        QFile checkFile(checkFileInfo.absoluteFilePath());
        checkFile.open(QIODevice::ReadOnly);
        QString readPath, readHash, bufferLine;
        int dataCnt;
        bufferLine = checkFile.readLine();
        dataCnt = bufferLine.toInt();
        while(dataCnt--) {
            readPath = checkFile.readLine().trimmed();
            readHash = checkFile.readLine().trimmed();
            LOG_INFO << "Reached new file " << readPath.toStdString() << " | Hash " << readHash.toStdString();
            pathHashMap.insert(pair<string, string>(readPath.toStdString() , readHash.toStdString()));
        }
        checkFile.close();
    }

    return true;
}

bool FileStorage::loadFSData()
{
    this->storageOf = PG_FILE_SRV;
    QString path = "FS_Index/.storage";
    QFileInfo checkFileInfo(path);
    if (checkFileInfo.exists() && checkFileInfo.isFile()) {
        QFile checkFile(checkFileInfo.absoluteFilePath());
        checkFile.open(QIODevice::ReadOnly);
        QString readHash;
        int readID, readPart;
        int dataCnt;
        dataCnt = checkFile.readLine().trimmed().toInt();
        while(dataCnt--) {
            readID = checkFile.readLine().trimmed().toInt();
            readHash = checkFile.readLine().trimmed();
            readPart = checkFile.readLine().trimmed().toInt();
            if (readID >= nextIndex) nextIndex = readID + 1;
            LOG_INFO << "ID " << readID << " | Hash " << readHash.toStdString() << " | Part " << readPart;
            pathHashMap.insert(pair<string, string>(to_string(readID) , readHash.toStdString()));
            idxPartMap[readID] = readPart;
        }
        checkFile.close();
    }

    return true;
}

bool FileStorage::saveISData()
{
    if (this->storageOf != PG_INFO_SRV) return false;

    lock_guard<mutex> guardian(writeFileMutex);

    QString path = "IS_Index/.storage";
    QFile saveFile(path);
    saveFile.open(QIODevice::WriteOnly);
    QTextStream out(&saveFile);

    out << pathHashMap.size() << endl;
    for (auto& kv : pathHashMap) {
        out << QString::fromStdString(kv.first).trimmed() << endl;  // path
        out << QString::fromStdString(kv.second).trimmed() << endl; // hash
    }
    saveFile.close();

    return true;
}

bool FileStorage::saveFSData()
{
    if (this->storageOf != PG_FILE_SRV) return false;

    lock_guard<mutex> guardian(writeFileMutex);

    QString path = "FS_Index/.storage";
    QFile saveFile(path);
    saveFile.open(QIODevice::WriteOnly);
    QTextStream out(&saveFile);

    out << pathHashMap.size() << endl;
    for (auto& kv : pathHashMap) {
        out << QString::fromStdString(kv.first).trimmed() << endl;  // path
        out << QString::fromStdString(kv.second).trimmed() << endl; // hash
        out << idxPartMap[stoi(kv.first)] << endl;
    }
    saveFile.close();

    return true;
}
