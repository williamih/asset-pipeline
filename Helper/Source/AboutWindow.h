#ifndef ABOUTWINDOW_H
#define ABOUTWINDOW_H

#include <QDialog>
#include <Pipeline/ProjectDBConn.h>

class AboutWindow : public QDialog {
    Q_OBJECT

public:
    AboutWindow(QWidget* parent = nullptr);

private:
};

#endif // ABOUTWINDOW_H
