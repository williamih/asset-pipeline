#ifndef HELPERAPP_H
#define HELPERAPP_H

#include <vector>
#include <functional>
#include <QObject>
#include <QByteArray>
#include <QTcpSocket>
#include <QMenu>
#include <Core/Types.h>
#include <IPCTypes.h>
#include "ProjectsWindow.h"

class HelperApp : public QObject {
    Q_OBJECT

public:
    HelperApp(u16 port, QObject* parent = nullptr);
    ~HelperApp();

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
    ProjectsWindow m_projectsWindow;
    std::vector<BytesSentFunc> m_callbackQueue;
};

#endif // HELPERAPP_H
