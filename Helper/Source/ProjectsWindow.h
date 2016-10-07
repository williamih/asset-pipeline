#ifndef PROJECTSWINDOW_H
#define PROJECTSWINDOW_H

#include <QWidget>
#include <Pipeline/ProjectDBConn.h>

class QTableWidget;
class QCheckBox;

class ProjectsWindow : public QWidget {
    Q_OBJECT

public:
    ProjectsWindow(QWidget* parent = nullptr);
    ~ProjectsWindow();

private slots:
    void AddProject();
    void SetAsActiveProject();

private:
    void ReloadTableData();
    void AddProjectInternal(const char* projName, const char* projDir);
    void OnCheckBoxClicked(int row);

    ProjectDBConn m_dbConn;
    std::vector<int> m_projectIDs;
    QTableWidget* m_tableWidget;
};

#endif // PROJECTSWINDOW_H
