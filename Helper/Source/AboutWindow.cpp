#include "AboutWindow.h"
#include <QBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QFile>

const char* VERSION_TEXT = "Version 1.0";

template<class T>
static void LoadTextFromFile(T& widget, const char* path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QByteArray ba = file.readAll();
    widget.setText(QString::fromUtf8(ba));
}

static void LoadCopyright(QLabel& label)
{
    LoadTextFromFile(label, ":/Resources/Copyright.txt");
}

static void LoadAcknowledgments(QTextEdit& textEdit)
{
    LoadTextFromFile(textEdit, ":/Resources/ThirdPartySoftware.html");
}

AboutWindow::AboutWindow(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("About Asset Pipeline");
    setFixedWidth(640);
    setFixedHeight(400);

    QVBoxLayout* overallLayout = new QVBoxLayout(this);
    setLayout(overallLayout);

    QLabel* lblAppName = new QLabel;
    lblAppName->setText("Asset Pipeline");
    lblAppName->setStyleSheet(
        "font-size: 30pt; "
        "font-family: \"Myriad Set Pro\", \"Helvetica Neue\", \"Helvetica\", \"Arial\", sans-serif; "
        "font-weight: 300; "
    );
    overallLayout->addWidget(lblAppName);

    QLabel* lblVersion = new QLabel;
    lblVersion->setText(VERSION_TEXT);
    overallLayout->addWidget(lblVersion);

    overallLayout->addSpacing(10);

    QLabel* lblCopyright = new QLabel;
    LoadCopyright(*lblCopyright);
    lblCopyright->setStyleSheet("font-size: 11pt; color: gray; ");
    overallLayout->addWidget(lblCopyright);

    overallLayout->addSpacing(20);

    QTextEdit* textEdit = new QTextEdit;
    textEdit->setReadOnly(true);
    textEdit->setAcceptRichText(true);
    LoadAcknowledgments(*textEdit);
    overallLayout->addWidget(textEdit);

    QPushButton* btnClose = new QPushButton("Close");
    connect(btnClose, &QPushButton::clicked, this, &QWidget::close);
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(btnClose);

    overallLayout->addLayout(buttonLayout);
}
