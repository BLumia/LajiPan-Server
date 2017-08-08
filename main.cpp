#include <QCoreApplication>
#include "testmain.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    TestMain app;
    QMetaObject::invokeMethod(&app, "main", Qt::QueuedConnection);

    return a.exec();
}
