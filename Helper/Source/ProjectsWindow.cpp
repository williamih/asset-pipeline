#include "ProjectsWindow.h"
#include <QBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QCheckBox>
#include <Core/Macros.h>
#include "AddProjectWindow.h"

ProjectsWindow::ProjectsWindow(QWidget* parent)
    : QWidget(parent)

    , m_dbConn()
    , m_projectIDs()
    , m_tableWidget(nullptr)
{
    setWindowTitle("Asset Pipeline");

    QPushButton* btnAddProject = new QPushButton("Add");
    QPushButton* btnEdit = new QPushButton("Edit");
    QPushButton* btnSetAsActiveProj = new QPushButton("Set as Active Project");
    QPushButton* btnClose = new QPushButton("Close");
    connect(btnAddProject, &QPushButton::clicked, this, &ProjectsWindow::AddProject);
    connect(btnClose, &QPushButton::clicked, this, &QWidget::close);
    connect(btnSetAsActiveProj, &QPushButton::clicked, this,
            &ProjectsWindow::SetAsActiveProject);

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->addWidget(btnAddProject);
    buttonsLayout->addWidget(btnEdit);
    buttonsLayout->addWidget(btnSetAsActiveProj);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(btnClose);

    m_tableWidget = new QTableWidget;
    m_tableWidget->setColumnCount(2);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    QHeaderView* horizHeader = m_tableWidget->horizontalHeader();
    horizHeader->setStretchLastSection(true);
    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_tableWidget->verticalHeader()->setDefaultSectionSize(24);
    QStringList headerLabels;
    headerLabels << "Active Status";
    headerLabels << "Project Name";
    m_tableWidget->setHorizontalHeaderLabels(headerLabels);

    QVBoxLayout* vBoxLayout = new QVBoxLayout;
    vBoxLayout->addWidget(m_tableWidget);
    vBoxLayout->addLayout(buttonsLayout);

    setLayout(vBoxLayout);

    ReloadTableData();
}

ProjectsWindow::~ProjectsWindow()
{}

void ProjectsWindow::AddProject()
{
    AddProjectWindow* win = new AddProjectWindow(this);
    win->setVisible(true);
    connect(win, &AddProjectWindow::accepted, [=] {
        QByteArray ba1 = win->GetProjName().toUtf8();
        QByteArray ba2 = win->GetProjDir().toUtf8();
        AddProjectInternal(ba1.data(), ba2.data());
        delete win;
    });
    connect(win, &AddProjectWindow::rejected, [=] {
        delete win;
    });
}

void ProjectsWindow::SetAsActiveProject()
{
    QList<QTableWidgetSelectionRange> list = m_tableWidget->selectedRanges();
    if (list.isEmpty()) {
        m_dbConn.SetActiveProjectID(-1);
    } else {
        int top = list[0].topRow();
        int bot = list[0].bottomRow();
        ASSERT(top == bot && "Multiple selection not allowed");
        m_dbConn.SetActiveProjectID(m_projectIDs[top]);
    }
    // TODO: Do this more efficiently than by just reloading the entire table!
    ReloadTableData();
}

static void SetCheckBox(QTableWidget* table, int row, QCheckBox* checkBox)
{
    QWidget* w = new QWidget;
    QHBoxLayout* l = new QHBoxLayout;
    l->setAlignment(Qt::AlignCenter);
    l->addWidget(checkBox);
    w->setLayout(l);
    l->setMargin(0);
    table->setCellWidget(row, 0, w);
}

static QCheckBox* GetCheckBox(QTableWidget* table, int row)
{
    QWidget* w = table->cellWidget(row, 0);
    QLayout* layout = w->layout();
    return (QCheckBox*)layout->itemAt(0)->widget();
}

void ProjectsWindow::ReloadTableData()
{
    m_dbConn.QueryAllProjectIDs(&m_projectIDs);

    m_tableWidget->setRowCount((int)m_projectIDs.size());

    for (size_t i = 0; i < m_projectIDs.size(); ++i) {
        QCheckBox* checkBox = new QCheckBox;
        checkBox->setChecked(m_dbConn.GetActiveProjectID() == m_projectIDs[i]);
        connect(checkBox, &QCheckBox::clicked, [=] {
            OnCheckBoxClicked((int)i);
        });

        SetCheckBox(m_tableWidget, (int)i, checkBox);

        if (!m_tableWidget->item(i, 1))
            m_tableWidget->setItem(i, 1, new QTableWidgetItem);

        QString str(m_dbConn.GetProjectName(m_projectIDs[i]).c_str());
        QTableWidgetItem* item = m_tableWidget->item(i, 1);
        item->setText(str);
        item->setFlags(item->flags() & ~(Qt::ItemIsEditable));
    }
}

void ProjectsWindow::AddProjectInternal(const char* projName, const char* projDir)
{
    m_dbConn.AddProject(projName, projDir);
    ReloadTableData();
}

void ProjectsWindow::OnCheckBoxClicked(int row)
{
    QCheckBox* checkBox = GetCheckBox(m_tableWidget, row);
    if (checkBox->isChecked()) {
        m_dbConn.SetActiveProjectID(m_projectIDs[row]);
    } else {
        m_dbConn.SetActiveProjectID(-1);
    }
    // TODO: Do this more efficiently than by just reloading the entire table!
    ReloadTableData();
}
