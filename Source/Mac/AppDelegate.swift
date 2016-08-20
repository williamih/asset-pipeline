import Cocoa

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate {

    @IBOutlet weak var window: NSWindow!

    let statusItem = NSStatusBar.systemStatusBar().statusItemWithLength(-2)
    let assetPipeline = AssetPipelineWrapper()

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

    func manageProjects(sender: AnyObject) {
        NSApp.activateIgnoringOtherApps(true)
        self.window.makeKeyAndOrderFront(nil)
        self.window.center()
    }

}
