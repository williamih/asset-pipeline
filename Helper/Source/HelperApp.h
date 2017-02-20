#ifndef HELPERAPP_H
#define HELPERAPP_H

#include <vector>
#include <functional>

#include <QObject>
#include <QByteArray>
#include <QTcpSocket>
#include <QMenu>

#include <Core/Types.h>
#include <Pipeline/ProjectDBConn.h>
#include <IPCTypes.h>

#include "ProjectsWindow.h"
#include "ErrorsWindow.h"
#include "AboutWindow.h"

class HelperApp : public QObject {
    Q_OBJECT

public:
    HelperApp(u16 port, QObject* parent = nullptr);
    ~HelperApp();

private slots:
    void ShowAboutWindow();

private:
    typedef std::function<void(size_t)> BytesSentFunc;

    void SocketReadyForRead();
    void ReceiveByte(u8 byte);
    void SendIPCMessage(IPCHelperToAppAction action);
    void RegisterOnBytesSent(const BytesSentFunc& func);
    void OnBytesWritten(qint64 bytes);

    QMenu m_menu;
    QTcpSocket m_tcpSocket;
    std::vector<u8> m_socketReadData;
    ProjectDBConn m_dbConn;
    ProjectsWindow m_projectsWindow;
    ErrorsWindow m_errorsWindow;
    AboutWindow m_aboutWindow;
    std::vector<BytesSentFunc> m_callbackQueue;
};

#endif // HELPERAPP_H
