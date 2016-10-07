#include "AddProjectWindow.h"

#include <QBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>

#include <Core/Macros.h>

const unsigned LINE_EDIT_WIDTH = 300;

AddProjectWindow::AddProjectWindow(QWidget* parent)
    : QDialog(parent, Qt::Sheet)
    , m_projNameLineEdit(nullptr)
    , m_projDirLineEdit(nullptr)
{
    ASSERT(parent);

    QLabel* label = new QLabel("Configure properties for the new project:");

    QWidget* groupBox = CreateGroupBox();

    QPushButton* btnCancel = new QPushButton("Cancel");
    QPushButton* btnCreate = new QPushButton("Create");
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnCreate, &QPushButton::clicked, this, &QDialog::accept);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(btnCancel);
    buttonLayout->addStretch();
    buttonLayout->addWidget(btnCreate);

    QVBoxLayout* overallLayout = new QVBoxLayout;
    overallLayout->addWidget(label);
    overallLayout->addWidget(groupBox);
    overallLayout->addLayout(buttonLayout);

    setLayout(overallLayout);
}

AddProjectWindow::~AddProjectWindow()
{}

void AddProjectWindow::SelectProjectDir()
{
    QString path = QFileDialog::getExistingDirectory(this);
    if (!path.isEmpty()) {
        m_projDirLineEdit->setText(path);
    }
}

QWidget* AddProjectWindow::CreateGroupBox()
{
    QWidget* groupBox = new QWidget;
    groupBox->setFixedWidth(500);
    groupBox->setFixedHeight(125);
    groupBox->setObjectName("groupBox");
    groupBox->setStyleSheet(
        "#groupBox { "
        "    background-color: white; "
        "    border-style: solid; "
        "    border-color: gray; "
        "    border-width: 1px; "
        "}"
    );

    QLabel* lblName = new QLabel("Name:");
    m_projNameLineEdit = new QLineEdit;
    m_projNameLineEdit->setFixedWidth(LINE_EDIT_WIDTH);

    QLabel* lblProjDir = new QLabel("Project Directory:");
    m_projDirLineEdit = new QLineEdit;
    m_projDirLineEdit->setFixedWidth(LINE_EDIT_WIDTH);
    m_projDirLineEdit->setReadOnly(true);

    QPushButton* btnSelectProjDir = new QPushButton("Select Project Directory...");
    btnSelectProjDir->setAutoDefault(false);
    btnSelectProjDir->setFixedWidth(LINE_EDIT_WIDTH);
    connect(btnSelectProjDir, &QPushButton::clicked, this, &AddProjectWindow::SelectProjectDir);

    QFormLayout* formLayout = new QFormLayout;
    formLayout->addRow(lblName, m_projNameLineEdit);
    formLayout->addRow(lblProjDir, m_projDirLineEdit);
    formLayout->addRow(new QLabel, btnSelectProjDir);

    QVBoxLayout* groupBoxLayout = new QVBoxLayout(groupBox);
    groupBoxLayout->addStretch();
    groupBoxLayout->addLayout(formLayout);
    groupBoxLayout->addStretch();

    return groupBox;
}

QString AddProjectWindow::GetProjName() const
{
    return m_projNameLineEdit->text();
}

QString AddProjectWindow::GetProjDir() const
{
    return m_projDirLineEdit->text();
}
