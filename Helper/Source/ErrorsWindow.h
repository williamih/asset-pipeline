#ifndef ERRORSWINDOW_H
#define ERRORSWINDOW_H

#include <QWidget>
#include <Pipeline/ProjectDBConn.h>

class QListWidget;
class QTextEdit;

class ProjectDBConn;

class ErrorsWindow : public QWidget {
    Q_OBJECT

public:
    ErrorsWindow(ProjectDBConn& dbConn, QWidget* parent = nullptr);
    ~ErrorsWindow();

    void ReloadErrors();

protected:
    virtual void showEvent(QShowEvent* event);

private slots:
    void OnErrorListRowChanged(int currentRow);

private:

    QWidget* CreateMasterPane();
    QWidget* CreateDetailPane();

    ProjectDBConn& m_dbConn;
    std::vector<int> m_errorIDs;
    QListWidget* m_errorList;
    QTextEdit* m_textEditInputFiles;
    QTextEdit* m_textEditOutputFiles;
    QTextEdit* m_textEditErrorMessage;
};

#endif // ERRORSWINDOW_H
