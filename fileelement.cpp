#include "fileelement.h"
#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QTextStream>
#include <muduo/base/Logging.h>

using namespace muduo;

FileElement::FileElement()
{
    this->inited = false;
}

bool FileElement::loadState(std::string md5)
{
    QString path = "IS_Index" + QString::fromStdString(md5);
    QFileInfo checkFileInfo(path);
    if (checkFileInfo.exists() && checkFileInfo.isFile()) {
        QFile checkFile(checkFileInfo.absoluteFilePath());
        checkFile.open(QIODevice::ReadOnly);
        QString buffer;
        buffer = checkFile.readLine();
        this->id = buffer.toInt();
        this->ff32b = checkFile.readLine();
        int chunkCnt = checkFile.readLine().toInt();
        for(int i = 1; i <= chunkCnt; i++) {
            this->chunkArray.push_back(checkFile.readLine().toInt());
        }
        checkFile.close();
        this->inited = true;
    }

    return this->inited;
}

bool FileElement::saveState()
{
    if (this->inited || (!this->md5.isEmpty() && !this->ff32b.isEmpty())) {
        if (this->chunkArray.size() == 0) {
            LOG_WARN << "File Element chunkArray.size() == 0";
        }
        if (this->notReadyChunkArray.size() != 0) {
            LOG_WARN << "File Element notReadyChunkArray.size() != 0";
        }
        QFile checkFile("IS_Index" + this->md5);
        QTextStream in(&checkFile);
        in << this->id << '\n';
        in << this->ff32b << '\n';
        in << this->chunkArray.size() << '\n';
        for (const int& i : this->chunkArray) {
            in << i << '\n';
        }
        checkFile.close();
        this->inited = true;
    }
    return this->inited;
}

bool FileElement::isBinaryFf32bEqual(std::string binFf32bBuffer)
{
    QByteArray arr(binFf32bBuffer.c_str(), 32);
    QString arrQstr(arr.toHex());
    return (arrQstr == this->ff32b);
}
