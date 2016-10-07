#ifndef SystemTrayApp_H
#define SystemTrayApp_H

#include <vector>
#include <functional>

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTcpServer>
#include <QTimer>

#include <Core/Types.h>
#include <IPCTypes.h>
#include <Pipeline/ProjectDBConn.h>
#include <Pipeline/AssetPipeline.h>

class SystemTrayApp : public QObject, private AssetPipelineDelegate {
    Q_OBJECT

public:
    SystemTrayApp(QObject* parent = nullptr);
    virtual ~SystemTrayApp();

private:
    virtual void OnAssetBuildFinished(
        int projectID,
        int nSucceeded,
        int nFailed
    );
    virtual void OnAssetFailedToCompile(const AssetCompileFailureInfo& info);

private slots:
    void PipelineEventTimerTick();
    void ShowAboutWindow();
    void ManageProjects();
    void Compile();
    void Quit();

private:
    typedef std::function<void(size_t)> BytesSentFunc;

    void SocketDisconnected();
    void SocketReadyForRead();
    void ReceiveByte(u8 byte);

    void SendIPCMessage(IPCAppToHelperAction action);
    void RegisterOnBytesSent(const BytesSentFunc& func);
    void OnBytesWritten(qint64 bytes);

    void LaunchHelperIfNeeded();
    void OnNewConnection();

    // Os-specific functions
    void LaunchHelper(unsigned short port);

    QMenu m_menu;
    QSystemTrayIcon m_systemTrayIcon;
    QTcpServer m_tcpServer;
    QTcpSocket* m_socket;
    std::vector<u8> m_sendBuffer;
    std::vector<u8> m_socketReadData;
    std::vector<BytesSentFunc> m_callbackQueue;

    QTimer m_pipelineEventTimer;

    ProjectDBConn m_dbConn;
    AssetPipeline m_assetPipeline;
};

#endif // SystemTrayApp_H
