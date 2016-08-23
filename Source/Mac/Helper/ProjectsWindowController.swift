import Cocoa

class ProjectsWindowController: NSObject, NSWindowDelegate,
                                NSTableViewDataSource, NSTableViewDelegate {

    @IBOutlet weak var window : NSWindow!
    @IBOutlet weak var tableView : NSTableView!

    @IBOutlet weak var creationWindow : NSWindow!
    @IBOutlet weak var projectNameField : NSTextField!
    @IBOutlet weak var projectDirectoryField : NSTextField!

    let dbConn = ProjectDBConnWrapper()

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
    func setAsActiveProject(sender: AnyObject) {
        let index = tableView.selectedRow
        // N.B. >index< may be -1; this is okay and produces the desired
        // behavior when passed to setActiveProjectIndex().
        dbConn.setActiveProjectIndex(index)
        tableView.reloadData()
        // Need to re-select as the selection is lost when calling reloadData()
        tableView.selectRowIndexes(NSIndexSet(index: index),
                                   byExtendingSelection: false)
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
        let name = projectNameField.stringValue
        let dir = projectDirectoryField.stringValue
        window.endSheet(creationWindow, returnCode: NSModalResponseOK)
        projectNameField.stringValue = ""
        projectDirectoryField.stringValue = ""

        dbConn.addProjectWithName(name, directory: dir)

        tableView.reloadData()
    }

    @IBAction
    func createProjectCancelClicked(sender: AnyObject) {
        window.endSheet(creationWindow, returnCode: NSModalResponseCancel)
        projectNameField.stringValue = ""
        projectDirectoryField.stringValue = ""
    }

    @IBAction
    func createProjectSelectProjectDirectory(sender: AnyObject) {
        let panel = NSOpenPanel()
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

    // MARK: NSTableViewDataSource methods

    func numberOfRowsInTableView(tableView: NSTableView) -> Int {
        return dbConn.numProjects()
    }

    // MARK: NSTableViewDelegate methods

    func checkBoxAction(sender: AnyObject?) {
        if let checkBox = sender as? NSButton {
            let row = Int(checkBox.identifier!)!
            let selectedRow = tableView.selectedRow
            if checkBox.state == NSOnState {
                dbConn.setActiveProjectIndex(row)
            } else {
                dbConn.setActiveProjectIndex(-1)
            }
            tableView.reloadData()
            // Have to re-select as reloadData() loses the selection
            tableView.selectRowIndexes(NSIndexSet(index: selectedRow),
                                       byExtendingSelection: false)
        }
    }

    func tableView(tableView: NSTableView,
                   viewForTableColumn tableColumn: NSTableColumn?,
                   row: Int) -> NSView? {
        if tableColumn == tableView.tableColumns[0] {
            if let cell = tableView.makeViewWithIdentifier("ActiveStatusID",
                                                           owner: nil)
                as? NSButton {

                if row == dbConn.activeProjectIndex() {
                    cell.state = NSOnState
                } else {
                    cell.state = NSOffState
                }
                cell.identifier = String(row)
                cell.target = self
                cell.action = #selector(checkBoxAction)
                return cell
            }
        }
        if tableColumn == tableView.tableColumns[1] {
            if let cell = tableView.makeViewWithIdentifier("ProjectNameID",
                                                           owner: nil)
                as? NSTableCellView {
                cell.textField?.stringValue = dbConn.nameOfProjectAtIndex(row)
                return cell
            }
        }
        return nil
    }

}
