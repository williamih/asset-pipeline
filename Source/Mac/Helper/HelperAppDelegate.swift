import Cocoa

@NSApplicationMain
class HelperAppDelegate: NSObject, NSApplicationDelegate, IPCConnectionDelegate {

    @IBOutlet weak var window: NSWindow!
    @IBOutlet weak var projectsWindowController : ProjectsWindowController!

    let menuAppConn = IPCConnection()

    func determinePort() -> UInt16 {
        let arguments = NSProcessInfo.processInfo().arguments;
        let value = Int(arguments[1]);
        if value != nil {
            if value < Int(UInt16.max) {
                return UInt16(value!)
            }
        }
        print("Invalid argument");
        abort();
    }

    func applicationDidFinishLaunching(aNotification: NSNotification) {
        menuAppConn.delegate = self
        menuAppConn.connectAsClientToPort(determinePort())
    }

    func applicationShouldTerminateAfterLastWindowClosed(sender: NSApplication)
        -> Bool {
        return true
    }

    @IBAction
    func quit(sender: AnyObject?) {
        menuAppConn.sendByte(IPCHelperToAppAction.Quit.rawValue)
    }

    func receiveIPCByte(byte: UInt8) {
        let action = IPCAppToHelperAction(rawValue: byte)!
        switch action {
        case .Quit:
            menuAppConn.sendByte(IPCHelperToAppAction.QuitReceived.rawValue,
                                 onComplete: {
                NSApp.terminate(nil)
            })
        case .QuitReceived:
            NSApp.terminate(nil)
        case .ShowProjectWindow:
            projectsWindowController.showWindow()
        }
    }

    func onIPCConnectionClosed() {
        NSApp.terminate(nil)
    }

}
