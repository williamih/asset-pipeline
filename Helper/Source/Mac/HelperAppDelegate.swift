import Cocoa

@NSApplicationMain
class HelperAppDelegate: NSObject, NSApplicationDelegate, IPCConnectionDelegate {

    @IBOutlet weak var window: NSWindow!
    @IBOutlet weak var projectsWindowController : ProjectsWindowController!

    let menuAppConn = IPCConnection()

    func determinePort() -> UInt16 {
        let arguments = ProcessInfo.processInfo.arguments;
        if let value = UInt16(arguments[1]) {
            return value
        }
        print("Invalid argument");
        abort();
    }

    func applicationDidFinishLaunching(_ aNotification: Notification) {
        menuAppConn.delegate = self
        menuAppConn.connectAsClient(toPort: determinePort())
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication)
        -> Bool {
        return true
    }

    @IBAction
    func quit(_ sender: AnyObject?) {
        menuAppConn.sendByte(IPCHelperToAppAction.quit.rawValue)
    }

    func receiveIPCByte(_ byte: UInt8) {
        let action = IPCAppToHelperAction(rawValue: byte)!
        switch action {
        case .quit:
            menuAppConn.sendByte(IPCHelperToAppAction.quitReceived.rawValue,
                                 onComplete: {
                NSApp.terminate(nil)
            })
        case .quitReceived:
            NSApp.terminate(nil)
        case .showProjectWindow:
            projectsWindowController.showWindow()
        }
    }

    func onIPCConnectionClosed() {
        NSApp.terminate(nil)
    }

}
