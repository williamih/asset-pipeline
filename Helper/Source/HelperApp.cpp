#include "HelperApp.h"
#include <vector>
#include <QApplication>
#include <QHostAddress>

#include <Core/Macros.h>

HelperApp::HelperApp(u16 port, QObject* parent)
    : QObject(parent)
    , m_menu()
    , m_tcpSocket()
    , m_socketReadData()
    , m_projectsWindow()
    , m_aboutWindow()
    , m_callbackQueue()
{
    QHostAddress address(QHostAddress::LocalHost);
    m_tcpSocket.connectToHost(address, port);
    connect(&m_tcpSocket, &QTcpSocket::readyRead,
            this, &HelperApp::SocketReadyForRead);
    connect(&m_tcpSocket, &QTcpSocket::bytesWritten,
            this, &HelperApp::OnBytesWritten);

    QAction* aboutAction = m_menu.addAction("About Asset Pipeline");
    aboutAction->setMenuRole(QAction::AboutRole);
    connect(aboutAction, &QAction::triggered, this, &HelperApp::ShowAboutWindow);

    QAction* quitAction = m_menu.addAction("Quit Asset Pipeline");
    quitAction->setMenuRole(QAction::QuitRole);
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, [=] {
        SendIPCMessage(IPCHELPERTOAPP_QUIT);
        RegisterOnBytesSent([](size_t bytes) {
            QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
        });
    });

    m_tcpSocket.waitForConnected();
}

HelperApp::~HelperApp()
{}

void HelperApp::ShowAboutWindow()
{
    m_aboutWindow.show();
}

void HelperApp::SocketReadyForRead()
{
    qint64 bytesAvailable = m_tcpSocket.bytesAvailable();
    m_socketReadData.resize((size_t)bytesAvailable);
    m_tcpSocket.read((char*)&m_socketReadData[0],
                     (qint64)m_socketReadData.size());

    for (size_t i = 0; i < m_socketReadData.size(); ++i) {
        ReceiveByte(m_socketReadData[i]);
    }
}

void HelperApp::ReceiveByte(u8 byte)
{
    IPCAppToHelperAction action = (IPCAppToHelperAction)byte;
    switch (action) {
        case IPCAPPTOHELPER_SHOW_PROJECTS_WINDOW:
            m_projectsWindow.show();
            break;
        case IPCAPPTOHELPER_SHOW_ABOUT_WINDOW:
            ShowAboutWindow();
            break;
        case IPCAPPTOHELPER_QUIT:
            QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
            break;
    }
}

void HelperApp::SendIPCMessage(IPCHelperToAppAction action)
{
    if ((u32)action > 0xFF)
        FATAL("Size of message exceeded one byte");
    u8 byte = (u8)action;
    m_tcpSocket.write((const char*)&byte, 1);
}

void HelperApp::RegisterOnBytesSent(const BytesSentFunc& func)
{
    m_callbackQueue.push_back(func);
}

void HelperApp::OnBytesWritten(qint64 bytes)
{
    for (size_t i = 0; i < m_callbackQueue.size(); ++i) {
        m_callbackQueue[i]((size_t)bytes);
    }
    m_callbackQueue.clear();
}
