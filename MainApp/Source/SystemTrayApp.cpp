#include "SystemTrayApp.h"
#include <QApplication>
#include <QMenu>
#include <QTcpSocket>

#include <Core/Macros.h>

// Milliseconds
const int PIPELINE_DELEGATE_MESSAGE_POLL_INTERVAL_MS = 20;

SystemTrayApp::SystemTrayApp(QObject* parent)
    : QObject(parent)

    , m_menu()
    , m_systemTrayIcon()
    , m_tcpServer()
    , m_socket(nullptr)
    , m_sendBuffer()
    , m_socketReadData()
    , m_callbackQueue()

    , m_pipelineEventTimer()

    , m_dbConn()
    , m_assetPipeline()
{
    m_systemTrayIcon.setIcon(QIcon(":/Resources/SystemTrayIcon.png"));
    m_systemTrayIcon.setVisible(true);

    m_systemTrayIcon.setContextMenu(&m_menu);

    connect(&m_tcpServer, &QTcpServer::newConnection,
            this, &SystemTrayApp::OnNewConnection);

    QAction* aboutAction = m_menu.addAction("About Asset Pipeline");
    connect(aboutAction, &QAction::triggered,
            this, &SystemTrayApp::ShowAboutWindow);

    m_menu.addSeparator();

    QAction* manageProjectsAction = m_menu.addAction("Manage Projects");
    connect(manageProjectsAction, &QAction::triggered,
            this, &SystemTrayApp::ManageProjects);

    QAction* viewErrorsAction = m_menu.addAction("View Error List");
    connect(viewErrorsAction, &QAction::triggered,
            this, &SystemTrayApp::ViewErrorList);

    QAction* compileAction = m_menu.addAction("Compile");
    connect(compileAction, &QAction::triggered, this, &SystemTrayApp::Compile);

    m_menu.addSeparator();

    QAction* quitAction = m_menu.addAction("Quit Asset Pipeline");
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, [=] {
        Quit();
    });

    connect(&m_pipelineEventTimer, &QTimer::timeout, this,
            &SystemTrayApp::PipelineEventTimerTick);
    m_pipelineEventTimer.start(PIPELINE_DELEGATE_MESSAGE_POLL_INTERVAL_MS);

    m_assetPipeline.SetDelegate(this);

    // Compile if possible.
    Compile();
}

SystemTrayApp::~SystemTrayApp()
{}

void SystemTrayApp::OnAssetBuildFinished(const AssetBuildCompletionInfo& info)
{
    std::string projName = m_dbConn.GetProjectName(info.projectID);

    QString title = QString("Asset Build Completed (%1)").arg(projName.c_str());

    QString message;
    if (info.nFailed == 0 && info.nSucceeded > 0) {
        message = QString("Successfully compiled %1%2 asset%3.")
                      .arg(info.nSucceeded == 1 ? "" : "all ")
                      .arg(info.nSucceeded)
                      .arg(info.nSucceeded == 1 ? "" : "s");
    }
    else if (info.nFailed == 0) {
        message = "All assets were already up to date.";
    }
    else {
        message = QString("%1 asset%2 compiled successfully, %3 failed")
                      .arg(info.nSucceeded)
                      .arg(info.nSucceeded == 1 ? "" : "s")
                      .arg(info.nFailed);
    }

    m_systemTrayIcon.showMessage(title, message);
}

void SystemTrayApp::OnAssetRecompileFinished(const AssetRecompileInfo& info)
{
    std::string projName = m_dbConn.GetProjectName(info.projectID);

    QString title;
    QString message(info.path.c_str());
    if (info.succeeded) {
        title = QString("Recompiled File in Project '%1'").arg(projName.c_str());
    } else {
        title = QString("Failed to Compile Asset (Project: %1)").arg(projName.c_str());
    }

    m_systemTrayIcon.showMessage(title, message);
}

void SystemTrayApp::OnAssetCompileSucceeded()
{
    if (IsConnectedToHelper())
        SendIPCMessage(IPCAPPTOHELPER_REFRESH_ERRORS);
}

void SystemTrayApp::OnAssetFailedToCompile(const AssetCompileFailureInfo& info)
{
    if (IsConnectedToHelper())
        SendIPCMessage(IPCAPPTOHELPER_REFRESH_ERRORS);
}

void SystemTrayApp::PipelineEventTimerTick()
{
    m_assetPipeline.CallDelegateFunctions();
}

void SystemTrayApp::ShowAboutWindow()
{
    LaunchHelperIfNeeded();
    SendIPCMessage(IPCAPPTOHELPER_SHOW_ABOUT_WINDOW);
}

void SystemTrayApp::ManageProjects()
{
    LaunchHelperIfNeeded();
    SendIPCMessage(IPCAPPTOHELPER_SHOW_PROJECTS_WINDOW);
}

void SystemTrayApp::ViewErrorList()
{
    LaunchHelperIfNeeded();
    SendIPCMessage(IPCAPPTOHELPER_SHOW_ERRORS_WINDOW);
}

void SystemTrayApp::Compile()
{
    int projID = m_dbConn.GetActiveProjectID();
    if (projID >= 0)
        m_assetPipeline.CompileProject(projID);
}

void SystemTrayApp::Quit()
{
    if (m_socket) {
        SendIPCMessage(IPCAPPTOHELPER_QUIT);
        RegisterOnBytesSent([](size_t bytes) {
            QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
        });
    } else {
        QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
    }
}

void SystemTrayApp::SocketDisconnected()
{
    m_socket->deleteLater();
    m_socket = nullptr;
}

void SystemTrayApp::SocketReadyForRead()
{
    qint64 bytesAvailable = m_socket->bytesAvailable();
    m_socketReadData.resize((size_t)bytesAvailable);
    m_socket->read((char*)&m_socketReadData[0],
                   (qint64)m_socketReadData.size());

    for (size_t i = 0; i < m_socketReadData.size(); ++i) {
        ReceiveByte(m_socketReadData[i]);
    }
}

void SystemTrayApp::ReceiveByte(u8 byte)
{
    IPCHelperToAppAction action = (IPCHelperToAppAction)byte;
    switch (action) {
        case IPCHELPERTOAPP_QUIT:
            QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
            break;
    }
}

void SystemTrayApp::SendIPCMessage(IPCAppToHelperAction action)
{
    ASSERT(IsConnectedToHelper());

    if ((u32)action > 0xFF)
        FATAL("Size of message exceeded one byte");
    u8 byte = (u8)action;
    if (m_socket) {
        m_socket->write((const char*)&byte, 1);
    } else {
        m_sendBuffer.push_back(byte);
    }
}

void SystemTrayApp::RegisterOnBytesSent(const BytesSentFunc& func)
{
    m_callbackQueue.push_back(func);
}

void SystemTrayApp::OnBytesWritten(qint64 bytes)
{
    for (size_t i = 0; i < m_callbackQueue.size(); ++i) {
        m_callbackQueue[i]((size_t)bytes);
    }
    m_callbackQueue.clear();
}

bool SystemTrayApp::IsConnectedToHelper() const
{
    return (m_socket || m_tcpServer.isListening());
}

void SystemTrayApp::LaunchHelperIfNeeded()
{
    if (IsConnectedToHelper())
        return;

    ASSERT(!m_tcpServer.isListening());
    m_tcpServer.listen();

    LaunchHelper(m_tcpServer.serverPort());
}

void SystemTrayApp::OnNewConnection()
{
    ASSERT(!m_socket);
    m_socket = m_tcpServer.nextPendingConnection();
    connect(m_socket, &QTcpSocket::disconnected,
            this, &SystemTrayApp::SocketDisconnected);
    connect(m_socket, &QTcpSocket::readyRead,
            this, &SystemTrayApp::SocketReadyForRead);
    connect(m_socket, &QTcpSocket::bytesWritten,
            this, &SystemTrayApp::OnBytesWritten);
    m_tcpServer.close();
    if (!m_sendBuffer.empty()) {
        m_socket->write((const char*)&m_sendBuffer[0],
                        (qint64)m_sendBuffer.size());
        m_sendBuffer.clear();
    }
}
