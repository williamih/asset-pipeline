import Cocoa

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate, IPCConnectionDelegate {

    let statusItem = NSStatusBar.systemStatusBar().statusItemWithLength(-2)
    let assetPipeline = AssetPipelineWrapper()
    var helperConn : IPCConnection?

    func applicationDidFinishLaunching(aNotification: NSNotification) {
        if let button = statusItem.button {
            button.image = NSImage(named: "StatusBarButtonImage")
        }

        let menu = NSMenu()

        menu.addItem(NSMenuItem(title: "Manage Projects",
            action: #selector(manageProjects),
            keyEquivalent: ""))
        menu.addItem(NSMenuItem.separatorItem())
        menu.addItem(NSMenuItem(title: "Quit Asset Pipeline",
                               action: #selector(quit),
                        keyEquivalent: "q"))

        statusItem.menu = menu
    }

    func quit(sender: AnyObject?) {
        if let conn = helperConn {
            conn.sendByte(IPCAppToHelperAction.Quit.rawValue)
        } else {
            NSApp.terminate(nil)
        }
    }

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

}
