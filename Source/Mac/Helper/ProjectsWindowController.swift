import Cocoa

class ProjectsWindowController: NSObject, NSWindowDelegate {

    @IBOutlet weak var window : NSWindow!
    @IBOutlet weak var tableView : NSTableView!

    func windowDidResignKey(notification: NSNotification) {
    }

    func showWindow() {
        NSApp.activateIgnoringOtherApps(true)
        window.makeKeyAndOrderFront(nil)
        window.center()
    }

    @IBAction
    func closeWindow(sender: AnyObject) {
        window.orderOut(nil)
    }

}
