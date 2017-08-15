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

FileElement::~FileElement()
{
    this->saveState();
}

bool FileElement::loadState(std::string md5)
{
    QString path = "IS_Index/" + QString::fromStdString(md5);
    QFileInfo checkFileInfo(path);
    if (checkFileInfo.exists() && checkFileInfo.isFile()) {
        QFile checkFile(checkFileInfo.absoluteFilePath());
        checkFile.open(QIODevice::ReadOnly);
        QString buffer;
        buffer = checkFile.readLine().trimmed();
        this->id = buffer.toInt();
        this->ff16b = checkFile.readLine().trimmed();
        this->totalChunkCount = checkFile.readLine().toInt();
        int chunkCnt = checkFile.readLine().toInt();
        for(int i = 1; i <= chunkCnt; i++) {
            this->chunkArray.push_back(checkFile.readLine().toInt());
        }
        checkFile.close();
        this->md5 = QString::fromStdString(md5);
        this->inited = true;
    }

    return this->inited;
}

bool FileElement::saveState()
{
    if (this->chunkArray.size() == 0) {
        LOG_WARN << "File Element chunkArray.size() == 0";
    }
    if (this->totalChunkCount != this->chunkArray.size()) {
        LOG_WARN << "Not all chunk of this File Element are ready!";
    }
    if (this->ff16b.isEmpty() || this->md5.isEmpty()) {
        LOG_WARN << "Not inited File Element, Will NOT Save!";
        return false;
    }

    QString path = "IS_Index/" + this->md5;
    QFile checkFile(path);
    checkFile.open(QFile::WriteOnly);
    QTextStream out(&checkFile);
    out << this->id << '\n';
    out << this->ff16b << '\n';
    out << this->totalChunkCount << '\n';
    out << this->chunkArray.size() << '\n';
    for (const int& i : this->chunkArray) {
        out << i << '\n';
    }
    checkFile.close();
    this->inited = true;

    return true;
}

bool FileElement::isBinaryFf16bEqual(std::string binFf16bBuffer)
{
    QByteArray arr(binFf16bBuffer.c_str(), 32);
    QString arrQstr(arr.toHex());
    return (arrQstr == this->ff16b);
}
