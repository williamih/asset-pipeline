#ifndef ADDPROJECTWINDOW_H
#define ADDPROJECTWINDOW_H

#include <QDialog>

class QLineEdit;

class AddProjectWindow : public QDialog {
    Q_OBJECT

public:
    AddProjectWindow(QWidget* parent = nullptr);
    ~AddProjectWindow();

    QString GetProjName() const;
    QString GetProjDir() const;

private slots:
    void SelectProjectDir();

private:
    QWidget* CreateGroupBox();

    QLineEdit* m_projNameLineEdit;
    QLineEdit* m_projDirLineEdit;
};

#endif // ADDPROJECTWINDOW_H
