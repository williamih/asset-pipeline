import Cocoa

class ProjectsWindowController: NSObject, NSWindowDelegate {

    @IBOutlet weak var window : NSWindow!
    @IBOutlet weak var tableView : NSTableView!

    @IBOutlet weak var creationWindow : NSWindow!
    @IBOutlet weak var projectNameField : NSTextField!
    @IBOutlet weak var projectDirectoryField : NSTextField!

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

    @IBAction
    func createProject(sender: AnyObject) {
        window.beginSheet(creationWindow,
                          completionHandler: { (returnCode) -> Void in

        })
        NSApp.activateIgnoringOtherApps(true)
        window.makeKeyAndOrderFront(nil)
    }

    @IBAction
    func createProjectCreateClicked(sender: AnyObject) {
        window.endSheet(creationWindow, returnCode: NSModalResponseOK)
    }

    @IBAction
    func createProjectCancelClicked(sender: AnyObject) {
        window.endSheet(creationWindow, returnCode: NSModalResponseCancel)
    }

    @IBAction
    func createProjectSelectProjectDirectory(sender: AnyObject) {
        let panel = NSOpenPanel();
        panel.allowsMultipleSelection = false
        panel.canChooseDirectories = true
        panel.canCreateDirectories = false
        panel.canChooseFiles = false
        panel.beginSheetModalForWindow(creationWindow,
                                       completionHandler: { (returnCode) -> Void in
            if returnCode == NSFileHandlingPanelOKButton {
                if let URL = panel.URL {
                    let str = URL.path!
                    self.projectDirectoryField.stringValue = str
                }
            }
        })
    }

}
