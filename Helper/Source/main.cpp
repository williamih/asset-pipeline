#include "ProjectsWindow.h"
#include <utility>
#include <QApplication>
#include <Core/Types.h>
#include "HelperApp.h"

static std::pair<bool, u16> ParsePort(int argc, char** argv)
{
    if (argc < 2)
        return std::make_pair(false, (u16)0);
    u16 port;
    if (sscanf(argv[1], "%hu", &port) != 1)
        return std::make_pair(false, (u16)0);
    return std::make_pair(true, port);
}

int main(int argc, char *argv[])
{
    std::pair<bool, u16> result = ParsePort(argc, argv);
    if (!result.first)
        return 1;

    QApplication a(argc, argv);

    HelperApp* app = new HelperApp(result.second);

    QObject::connect(&a, &QApplication::aboutToQuit, [=] {
        delete app;
    });

    return a.exec();
}
