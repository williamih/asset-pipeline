#include <QApplication>
#include "SystemTrayApp.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    SystemTrayApp* systemTrayApp = new SystemTrayApp;

    QObject::connect(&a, &QApplication::aboutToQuit, [=] {
        delete systemTrayApp;
    });

    return a.exec();
}
