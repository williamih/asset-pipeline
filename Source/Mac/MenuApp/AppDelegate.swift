import Cocoa

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate {

    let statusItem = NSStatusBar.systemStatusBar().statusItemWithLength(-2)
    let assetPipeline = AssetPipelineWrapper()
    var helperConn : IPCConnection?

    func applicationDidFinishLaunching(aNotification: NSNotification) {
        if let button = statusItem.button {
            button.image = NSImage(named: "StatusBarButtonImage")
            button.action = #selector(statusBarButtonClicked)
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

    func applicationWillTerminate(aNotification: NSNotification) {
        // Insert code here to tear down your application
    }

    func statusBarButtonClicked(sender: AnyObject) {
        print("Hello world")
    }

    func quit(sender: AnyObject) {
        NSApp.terminate(self);
    }

    func launchHelperIfNeeded() {
        if helperConn != nil {
            return;
        }

        helperConn = IPCConnection()
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
