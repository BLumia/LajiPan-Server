#ifndef TESTMAIN_H
#define TESTMAIN_H

#include <QObject>
#include <SimpleIni.h>
#include "common.h"

class TestMain : public QObject
{
    Q_OBJECT
public:
    CSimpleIniA conf;
    Common sharedData;
    explicit TestMain(QObject *parent = nullptr);
    ~TestMain();

signals:

public slots:
    void main();
};

#endif // TESTMAIN_H
