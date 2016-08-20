import Cocoa

class ProjectsWindowController: NSObject, NSWindowDelegate {

    @IBOutlet weak var window : NSWindow!
    @IBOutlet weak var tableView : NSTableView!

    func windowDidResignKey(notification: NSNotification) {
        NSApp.hide(nil)
        NSApp.unhideWithoutActivation()
    }

    @IBAction
    func closeWindow(sender: AnyObject) {
        window.orderOut(nil)
    }

}
