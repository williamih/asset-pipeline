#include "ErrorsWindow.h"
#include <QBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QTextEdit>
#include <QGroupBox>
#include <Core/Macros.h>
#include <Pipeline/ProjectDBConn.h>

ErrorsWindow::ErrorsWindow(ProjectDBConn& dbConn, QWidget* parent)
    : QWidget(parent)

    , m_dbConn(dbConn)
    , m_errorIDs()
    , m_errorList(nullptr)
    , m_textEditInputFiles(nullptr)
    , m_textEditOutputFiles(nullptr)
    , m_textEditErrorMessage(nullptr)
{
    setWindowTitle("Asset Pipeline - Error List");
    resize(800, 500);

    QHBoxLayout* overallLayout = new QHBoxLayout(this);
    overallLayout->addWidget(CreateMasterPane());
    overallLayout->addWidget(CreateDetailPane());

    setLayout(overallLayout);
}

ErrorsWindow::~ErrorsWindow()
{}

QWidget* ErrorsWindow::CreateMasterPane()
{
    ASSERT(!m_errorList);

    m_errorList = new QListWidget;
    m_errorList->setFixedWidth(250);
    m_errorList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_errorList, &QListWidget::currentRowChanged,
            this, &ErrorsWindow::OnErrorListRowChanged);

    return m_errorList;
}

QWidget* ErrorsWindow::CreateDetailPane()
{
    ASSERT(!m_textEditInputFiles);

    QGroupBox* groupBox = new QGroupBox;
    QVBoxLayout* groupBoxLayout = new QVBoxLayout(groupBox);
    groupBox->setLayout(groupBoxLayout);

    QLabel* lblInputFiles = new QLabel("Input Files:");
    m_textEditInputFiles = new QTextEdit;
    m_textEditInputFiles->setFixedHeight(50);
    m_textEditInputFiles->setReadOnly(true);

    QLabel* lblOutputFiles = new QLabel("Output Files:");
    m_textEditOutputFiles = new QTextEdit;
    m_textEditOutputFiles->setFixedHeight(50);
    m_textEditOutputFiles->setReadOnly(true);

    QLabel* lblErrorMessage = new QLabel("Message:");
    m_textEditErrorMessage = new QTextEdit;
    m_textEditErrorMessage->setReadOnly(true);

    groupBoxLayout->addWidget(lblInputFiles);
    groupBoxLayout->addWidget(m_textEditInputFiles);
    groupBoxLayout->addWidget(lblOutputFiles);
    groupBoxLayout->addWidget(m_textEditOutputFiles);
    groupBoxLayout->addWidget(lblErrorMessage);
    groupBoxLayout->addWidget(m_textEditErrorMessage);

    return groupBox;
}

void ErrorsWindow::ReloadErrors()
{
    int activeProjID = m_dbConn.GetActiveProjectID();
    if (activeProjID >= 0)
        m_dbConn.QueryAllErrorIDs(activeProjID, &m_errorIDs);

    m_errorList->clear();

    std::vector<std::string> inputFiles;
    for (size_t i = 0; i < m_errorIDs.size(); ++i) {
        m_dbConn.GetErrorInputPaths(m_errorIDs[i], &inputFiles);
        if (inputFiles.size() == 0)
            continue;
        QString label(inputFiles[0].c_str());
        m_errorList->addItem(label);
    }

    if (m_errorList->selectedItems().isEmpty() && m_errorList->count() > 0) {
        m_errorList->setCurrentRow(0);
        m_errorList->setItemSelected(m_errorList->item(0), true);
    }
    if (m_errorList->count() == 0) {
        m_textEditInputFiles->setText("");
        m_textEditOutputFiles->setText("");
        m_textEditErrorMessage->setText("");
    }
}

void ErrorsWindow::showEvent(QShowEvent* event)
{
    ReloadErrors();
}

static void TextEdit_SetStringList(QTextEdit& textEdit,
                                   const std::vector<std::string>& vec)
{
    QString str;
    for (size_t i = 0; i < vec.size(); ++i) {
        str.append(vec[i].c_str());
        str.append('\n');
    }
    textEdit.setText(str);
}

void ErrorsWindow::OnErrorListRowChanged(int currentRow)
{
    ASSERT(m_textEditInputFiles);
    ASSERT(m_textEditOutputFiles);
    ASSERT(m_textEditErrorMessage);

    // HACK: This seems to happen reasonably often.
    if (currentRow < 0 || currentRow >= m_errorIDs.size())
        return;

    int errorID = m_errorIDs[currentRow];

    std::string errorMessage = m_dbConn.GetErrorMessage(errorID);
    m_textEditErrorMessage->setText(errorMessage.c_str());

    std::vector<std::string> vec;
    m_dbConn.GetErrorInputPaths(errorID, &vec);
    TextEdit_SetStringList(*m_textEditInputFiles, vec);
    m_dbConn.GetErrorOutputPaths(errorID, &vec);
    TextEdit_SetStringList(*m_textEditOutputFiles, vec);
}
