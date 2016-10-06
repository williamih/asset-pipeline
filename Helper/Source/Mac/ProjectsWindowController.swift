import Cocoa

class ProjectsWindowController: NSObject, NSWindowDelegate,
                                NSTableViewDataSource, NSTableViewDelegate {

    @IBOutlet weak var window : NSWindow!
    @IBOutlet weak var tableView : NSTableView!

    @IBOutlet weak var creationWindow : NSWindow!
    @IBOutlet weak var projectNameField : NSTextField!
    @IBOutlet weak var projectDirectoryField : NSTextField!

    let dbConn = ProjectDBConnWrapper()
    var projectIDs : [NSNumber]!

    override func awakeFromNib() {
        projectIDs = dbConn.queryAllProjectIDs()
    }

    func showWindow() {
        NSApp.activate(ignoringOtherApps: true)
        window.makeKeyAndOrderFront(nil)
        window.center()
    }

    @IBAction
    func closeWindow(_ sender: AnyObject) {
        window.orderOut(nil)
    }

    @IBAction
    func setAsActiveProject(_ sender: AnyObject) {
        let index = tableView.selectedRow
        var projID = -1
        if index != -1 {
            projID = Int(projectIDs[index])
        }
        dbConn.setActiveProjectID(projID)
        tableView.reloadData()
        // Need to re-select as the selection is lost when calling reloadData()
        tableView.selectRowIndexes(IndexSet(integer: index),
                                   byExtendingSelection: false)
    }

    @IBAction
    func createProject(_ sender: AnyObject) {
        window.beginSheet(creationWindow,
                          completionHandler: { (returnCode) -> Void in

        })
        NSApp.activate(ignoringOtherApps: true)
        window.makeKeyAndOrderFront(nil)
    }

    @IBAction
    func createProjectCreateClicked(_ sender: AnyObject) {
        let name = projectNameField.stringValue
        let dir = projectDirectoryField.stringValue
        window.endSheet(creationWindow, returnCode: NSModalResponseOK)
        projectNameField.stringValue = ""
        projectDirectoryField.stringValue = ""

        dbConn.addProject(withName: name, directory: dir)
        projectIDs = dbConn.queryAllProjectIDs()

        tableView.reloadData()
    }

    @IBAction
    func createProjectCancelClicked(_ sender: AnyObject) {
        window.endSheet(creationWindow, returnCode: NSModalResponseCancel)
        projectNameField.stringValue = ""
        projectDirectoryField.stringValue = ""
    }

    @IBAction
    func createProjectSelectProjectDirectory(_ sender: AnyObject) {
        let panel = NSOpenPanel()
        panel.allowsMultipleSelection = false
        panel.canChooseDirectories = true
        panel.canCreateDirectories = false
        panel.canChooseFiles = false
        panel.beginSheetModal(for: creationWindow,
                completionHandler: { (returnCode) -> Void in
            if returnCode == NSFileHandlingPanelOKButton {
                if let URL = panel.url {
                    let str = URL.path
                    self.projectDirectoryField.stringValue = str
                }
            }
        })
    }

    // MARK: NSTableViewDataSource methods

    func numberOfRows(in tableView: NSTableView) -> Int {
        return dbConn.numProjects()
    }

    // MARK: NSTableViewDelegate methods

    func checkBoxAction(_ sender: AnyObject?) {
        if let checkBox = sender as? NSButton {
            let row = Int(checkBox.identifier!)!
            let selectedRow = tableView.selectedRow
            if checkBox.state == NSOnState {
                let projID = Int(projectIDs[row])
                dbConn.setActiveProjectID(projID)
            } else {
                dbConn.setActiveProjectID(-1)
            }
            tableView.reloadData()
            // Have to re-select as reloadData() loses the selection
            tableView.selectRowIndexes(IndexSet(integer: selectedRow),
                                       byExtendingSelection: false)
        }
    }

    func tableView(_ tableView: NSTableView,
           viewFor tableColumn: NSTableColumn?,
                           row: Int) -> NSView? {
        if tableColumn == tableView.tableColumns[0] {
            if let cell = tableView.make(withIdentifier: "ActiveStatusID",
                                                           owner: nil)
                as? NSButton {

                let projID = Int(projectIDs[row])
                if projID == dbConn.activeProjectID() {
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
            if let cell = tableView.make(withIdentifier: "ProjectNameID",
                                                           owner: nil)
                as? NSTableCellView {
                let projID = Int(projectIDs[row])
                cell.textField?.stringValue = dbConn.nameOfProject(withID: projID)
                return cell
            }
        }
        return nil
    }

}
