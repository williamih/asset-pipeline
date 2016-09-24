import Cocoa

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate, NSUserNotificationCenterDelegate,
                   IPCConnectionDelegate, AssetPipelineMacDelegate {

    let statusItem = NSStatusBar.system().statusItem(withLength: -2)
    let dbConn = ProjectDBConnWrapper()
    let assetPipeline = AssetPipelineWrapper()
    var helperConn : IPCConnection?

    func pollAssetPipelineMessages(_ timer: Timer) {
        assetPipeline.pollMessages()
    }

    // MARK: NSApplicationDelegate methods

    func applicationDidFinishLaunching(_ aNotification: Notification) {
        let POLL_TIME_INTERVAL = 0.25 // seconds
        Timer.scheduledTimer(
            timeInterval: POLL_TIME_INTERVAL,
                  target: self,
                selector: #selector(pollAssetPipelineMessages),
                userInfo: nil,
                 repeats: true
        )

        if let button = statusItem.button {
            button.image = NSImage(named: "StatusBarButtonImage")
        }

        let menu = NSMenu()

        menu.addItem(NSMenuItem(title: "Manage Projects",
            action: #selector(manageProjects),
            keyEquivalent: ""))
        menu.addItem(NSMenuItem(title: "Compile",
            action: #selector(compile),
            keyEquivalent: ""))
        menu.addItem(NSMenuItem.separator())
        menu.addItem(NSMenuItem(title: "Quit Asset Pipeline",
                               action: #selector(quit),
                        keyEquivalent: "q"))

        statusItem.menu = menu

        NSUserNotificationCenter.default.delegate = self
        assetPipeline.delegate = self
    }

    // MARK: IPCConnectionDelegate methods

    func receiveIPCByte(_ byte: UInt8) {
        let action = IPCHelperToAppAction(rawValue: byte)!
        switch action {
        case .quit:
            helperConn!.sendByte(IPCAppToHelperAction.quitReceived.rawValue,
                                 onComplete: {
                NSApp.terminate(nil)
            })
        case .quitReceived:
            NSApp.terminate(nil)
        }
    }

    func onIPCConnectionClosed() {
        helperConn = nil
    }

    // MARK: AssetPipelineMacDelegate methods

    func assetCompileFinished(withSuccessCount successCount: Int,
                                               failureCount: Int) {
        let notification = NSUserNotification()
        notification.title = "Asset Build Completed"
        if (failureCount == 0 && successCount > 0) {
            notification.subtitle = String(
                format: "Successfully compiled all %d assets.",
                successCount
            )
        } else if (successCount == 0) {
            notification.subtitle = String(
                format: "All assets were already up to date."
            )
        } else {
            notification.subtitle = String(
                format: "%d assets compiled successfully, %d failed.",
                successCount,
                failureCount
            )
        }
        let center = NSUserNotificationCenter.default
        center.deliver(notification)
    }

    func assetCompileFailed(withInputPaths inputPaths: [String],
                                          outputPaths: [String],
                                         errorMessage: String) {
        let notification = NSUserNotification()
        notification.title = "Failed to Compile Asset"
        notification.subtitle = String(format: "%@", inputPaths[0])
        let center = NSUserNotificationCenter.default
        center.deliver(notification)
    }

    // MARK: NSUserNotificationCenterDelegate methods

    func userNotificationCenter(_ center: NSUserNotificationCenter,
              shouldPresent notification: NSUserNotification) -> Bool {
        return true
    }

    // MARK: Other methods

    func quit(_ sender: AnyObject?) {
        if let conn = helperConn {
            conn.sendByte(IPCAppToHelperAction.quit.rawValue)
        } else {
            NSApp.terminate(nil)
        }
    }

    func launchHelperIfNeeded() {
        if helperConn != nil {
            return;
        }

        helperConn = IPCConnection()
        helperConn!.delegate = self
        helperConn!.listenAsServer(onPort: 0)

        let port = helperConn!.port()

        let URL = Bundle.main.url(forResource: "Asset Pipeline Helper",
                                                       withExtension: ".app")
        let config = [
            NSWorkspaceLaunchConfigurationArguments : [ String(port) ]
        ]

        try! NSWorkspace.shared().launchApplication(
            at: URL!,
            options: NSWorkspaceLaunchOptions(),
            configuration: config
        )
    }

    func manageProjects(_ sender: AnyObject) {
        launchHelperIfNeeded()
        helperConn!.sendByte(IPCAppToHelperAction.showProjectWindow.rawValue)
    }

    func compile(_ sender: AnyObject) {
        let projIndex = dbConn.activeProjectIndex()
        if projIndex >= 0 {
            assetPipeline.compileProject(with: UInt(projIndex))
        }
    }
}
