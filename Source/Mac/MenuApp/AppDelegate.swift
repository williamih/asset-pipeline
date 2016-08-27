import Cocoa

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate, NSUserNotificationCenterDelegate,
                   IPCConnectionDelegate, AssetPipelineMacDelegate {

    let statusItem = NSStatusBar.systemStatusBar().statusItemWithLength(-2)
    let assetPipeline = AssetPipelineWrapper()
    let dbConn = ProjectDBConnWrapper()
    var helperConn : IPCConnection?

    func pollAssetPipelineMessages(timer: NSTimer) {
        assetPipeline.pollMessages()
    }

    // MARK: NSApplicationDelegate methods

    func applicationDidFinishLaunching(aNotification: NSNotification) {
        let POLL_TIME_INTERVAL = 0.25 // seconds
        NSTimer.scheduledTimerWithTimeInterval(
            POLL_TIME_INTERVAL,
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
        menu.addItem(NSMenuItem.separatorItem())
        menu.addItem(NSMenuItem(title: "Quit Asset Pipeline",
                               action: #selector(quit),
                        keyEquivalent: "q"))

        statusItem.menu = menu

        NSUserNotificationCenter.defaultUserNotificationCenter().delegate = self
        assetPipeline.delegate = self
    }

    // MARK: IPCConnectionDelegate methods

    func receiveIPCByte(byte: UInt8) {
        let action = IPCHelperToAppAction(rawValue: byte)!
        switch action {
        case .Quit:
            helperConn!.sendByte(IPCAppToHelperAction.QuitReceived.rawValue,
                                 onComplete: {
                NSApp.terminate(nil)
            })
        case .QuitReceived:
            NSApp.terminate(nil)
        }
    }

    func onIPCConnectionClosed() {
        helperConn = nil
    }

    // MARK: AssetPipelineMacDelegate methods

    func assetCompileFinishedWithSuccessCount(successCount: Int,
                                              failureCount: Int) {
        let notification = NSUserNotification()
        notification.title = "Asset Build Completed"
        if (failureCount == 0) {
            notification.subtitle = String(
                format: "Successfully compiled all %d assets.",
                successCount
            )
        } else {
            notification.subtitle = String(
                format: "%d assets compiled successfully, %d failed.",
                successCount,
                failureCount
            )
        }
        let center = NSUserNotificationCenter.defaultUserNotificationCenter()
        center.deliverNotification(notification)
    }

    func assetCompileFailedWithInputPaths(inputPaths: [String],
                                          outputPaths: [String],
                                          errorMessage: String) {
        let notification = NSUserNotification()
        notification.title = "Failed to Compile Asset"
        notification.subtitle = String(format: "%@", inputPaths[0])
        let center = NSUserNotificationCenter.defaultUserNotificationCenter()
        center.deliverNotification(notification)
    }

    // MARK: NSUserNotificationCenterDelegate methods

    func userNotificationCenter(center: NSUserNotificationCenter,
                                shouldPresentNotification notification: NSUserNotification) -> Bool {
        return true
    }

    // MARK: Other methods

    func quit(sender: AnyObject?) {
        if let conn = helperConn {
            conn.sendByte(IPCAppToHelperAction.Quit.rawValue)
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
        helperConn!.listenAsServerOnPort(0)

        let port = helperConn!.port()

        let URL = NSBundle.mainBundle().URLForResource("Asset Pipeline Helper",
                                                       withExtension: ".app")
        let config = [
            NSWorkspaceLaunchConfigurationArguments : [ String(port) ]
        ]

        try! NSWorkspace.sharedWorkspace().launchApplicationAtURL(
            URL!,
            options: NSWorkspaceLaunchOptions(),
            configuration: config
        )
    }

    func manageProjects(sender: AnyObject) {
        launchHelperIfNeeded()
        helperConn!.sendByte(IPCAppToHelperAction.ShowProjectWindow.rawValue)
    }

    func compile(sender: AnyObject) {
        let projIndex = dbConn.activeProjectIndex()
        let dir = dbConn.directoryOfProjectAtIndex(projIndex)
        assetPipeline.setProjectWithDirectory(dir)
        assetPipeline.compile()
    }
}
