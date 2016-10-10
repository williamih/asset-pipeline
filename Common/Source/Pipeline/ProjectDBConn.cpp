#include "ProjectDBConn.h"

#include <string.h>

#include <sqlite3/sqlite3.h>
#include <md5/md5.h>

#include <Core/Macros.h>
#include <Core/Types.h>

#include "AssetPipelineOsFuncs.h"

ProjectDBConn::DBHandle::DBHandle(const std::string& path)
    : db(NULL)
{
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK)
        FATAL("sqlite3_open");
}

ProjectDBConn::DBHandle::~DBHandle()
{
    if (sqlite3_close(db) != SQLITE_OK)
        FATAL("sqlite3_close");
}

i64 ProjectDBConn::DBHandle::LastInsertRowID() const
{
    return (i64)sqlite3_last_insert_rowid(db);
}

ProjectDBConn::SQLiteStatement::SQLiteStatement(DBHandle& db, const char* text,
                                                int nBytes, bool exec)
    : stmt(NULL)
{
    if (sqlite3_prepare_v2(db.db, text, nBytes, &stmt, NULL) != SQLITE_OK)
        FATAL("sqlite3_prepare_v2: %s", sqlite3_errmsg(db.db));
    if (exec)
        Exec(db);
}

ProjectDBConn::SQLiteStatement::~SQLiteStatement()
{
    if (sqlite3_finalize(stmt) != SQLITE_OK)
        FATAL("sqlite3_finalize");
}

void ProjectDBConn::SQLiteStatement::Exec(const DBHandle& db)
{
    if (sqlite3_step(stmt) != SQLITE_DONE)
        FATAL("sqlite3_step: %s", sqlite3_errmsg(db.db));
    Reset(db);
}

bool ProjectDBConn::SQLiteStatement::GetNextRow(const DBHandle& db)
{
    int ret = sqlite3_step(stmt);
    if (ret == SQLITE_ROW)
        return true;
    if (ret == SQLITE_DONE) {
        Reset(db);
        return false;
    }
    FATAL("sqlite3_step: %s", sqlite3_errmsg(db.db));
    return false;
}

void ProjectDBConn::SQLiteStatement::Reset(const DBHandle& db)
{
    if (sqlite3_reset(stmt) != SQLITE_OK)
        FATAL("sqlite3_reset: %s", sqlite3_errmsg(db.db));
}

void ProjectDBConn::SQLiteStatement::BindNull(int pos)
{
    if (sqlite3_bind_null(stmt, 1) != SQLITE_OK)
        FATAL("sqlite3_bind_null");
}

void ProjectDBConn::SQLiteStatement::BindInt(int pos, int value)
{
    if (sqlite3_bind_int(stmt, pos, value) != SQLITE_OK)
        FATAL("sqlite3_bind_int");
}

void ProjectDBConn::SQLiteStatement::BindInt64(int pos, i64 value)
{
    if (sqlite3_bind_int64(stmt, pos, (sqlite3_int64)value) != SQLITE_OK)
        FATAL("sqlite3_bind_int64");
}

void ProjectDBConn::SQLiteStatement::BindText(int pos, const char* str)
{
    if (sqlite3_bind_text(stmt, pos, strdup(str), -1, free) != SQLITE_OK)
        FATAL("sqlite3_bind_text");
}

int ProjectDBConn::SQLiteStatement::ColumnInt(int index)
{
    if (sqlite3_column_type(stmt, index) != SQLITE_INTEGER)
        FATAL("Wrong column type");
    return sqlite3_column_int(stmt, index);
}

const char* ProjectDBConn::SQLiteStatement::ColumnText(int index)
{
    if (sqlite3_column_type(stmt, index) != SQLITE_TEXT)
        FATAL("Wrong column type");
    return (const char*)sqlite3_column_text(stmt, index);
}

bool ProjectDBConn::SQLiteStatement::IsColumnNull(int index)
{
    return sqlite3_column_type(stmt, index) == SQLITE_NULL;
}

static const char STMT_BEGINTRANSAC[] = "BEGIN";
static const char STMT_ENDTRANSAC[] = "COMMIT";

static const char STMT_PROJECTSTABLE[] =
    "CREATE TABLE IF NOT EXISTS Projects ("
    "    ProjectID INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    Name TEXT NOT NULL,"
    "    Directory TEXT NOT NULL"
    ")";

static const char STMT_CONFIGTABLE[] =
    "CREATE TABLE IF NOT EXISTS Config ("
    "    ActiveProject INTEGER,"
    "    FOREIGN KEY(ActiveProject) REFERENCES Projects(ProjectID)"
    ")";

static const char STMT_DEPSTABLE[] =
    "CREATE TABLE IF NOT EXISTS Dependencies ("
    "    ProjectID INTEGER NOT NULL,"
    "    InputPath TEXT NOT NULL,"
    "    OutputPath TEXT NOT NULL,"
    "    FOREIGN KEY(ProjectID) REFERENCES Projects(ProjectID)"
    ")";

static const char STMT_ERRORSTABLE[] =
    "CREATE TABLE IF NOT EXISTS Errors ("
    "    ErrorID INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    ProjectID INTEGER NOT NULL,"
    "    Hash INTEGER NOT NULL,"
    "    Message TEXT NOT NULL,"
    "    FOREIGN KEY(ProjectID) REFERENCES Projects(ProjectID)"
    ")";

static const char STMT_ERRORINPUTSTABLE[] =
    "CREATE TABLE IF NOT EXISTS ErrorInputs ("
    "    ErrorID INTEGER NOT NULL,"
    "    InputPath TEXT NOT NULL,"
    "    FOREIGN KEY(ErrorID) REFERENCES Errors(ErrorID)"
    ")";

static const char STMT_ERROROUTPUTSTABLE[] =
    "CREATE TABLE IF NOT EXISTS ErrorOutputs ("
    "    ErrorID INTEGER NOT NULL,"
    "    OutputPath TEXT NOT NULL,"
    "    FOREIGN KEY(ErrorID) REFERENCES Errors(ErrorID)"
    ")";

static const char STMT_SETUPCONFIG[] =
    "INSERT INTO Config (ActiveProject) "
    "SELECT null "
    "WHERE NOT EXISTS (SELECT * FROM Config)";

static const char STMT_NUMPROJECTS[] = "SELECT COUNT(*) FROM Projects";

static const char STMT_QUERYALLPROJECTS[] = "SELECT ProjectID FROM Projects";

static const char STMT_PROJECTNAME[] = "SELECT Name FROM Projects"
                                       " WHERE ProjectID = ?";

static const char STMT_PROJECTDIR[] = "SELECT Directory FROM Projects"
                                      " WHERE ProjectID = ?";

static const char STMT_ADDPROJECT[] = "INSERT INTO Projects (Name, Directory)"
                                      " VALUES (?, ?)";

static const char STMT_GETACTIVEPROJ[] = "SELECT ActiveProject FROM Config";

static const char STMT_SETACTIVEPROJ[] = "UPDATE Config SET ActiveProject = ?";

static const char STMT_CLEARDEPS[] = "DELETE FROM Dependencies"
                                     " WHERE ProjectID = ? AND OutputPath = ?";

static const char STMT_RECORDDEP[] = "INSERT INTO Dependencies (ProjectID, InputPath, OutputPath)"
                                     " VALUES (?, ?, ?)";

static const char STMT_GETDEPS[] = "SELECT OutputPath FROM Dependencies"
                                   " WHERE ProjectID = ? AND InputPath = ?";

static const char STMT_FETCHERRORS[] = "SELECT ErrorID FROM Errors"
                                       " WHERE ProjectID = ? AND Hash = ?";

static const char STMT_ERRORGETINPUTS[] = "SELECT InputPath FROM ErrorInputs"
                                          " WHERE ErrorID = ?";

static const char STMT_ERRORGETOUTPUTS[] = "SELECT OutputPath FROM ErrorOutputs"
                                           " WHERE ErrorID = ?";

static const char STMT_ERROR_DELETE1[] = "DELETE FROM ErrorInputs WHERE ErrorID = ?";
static const char STMT_ERROR_DELETE2[] = "DELETE FROM ErrorOutputs WHERE ErrorID = ?";
static const char STMT_ERROR_DELETE3[] = "DELETE FROM Errors WHERE ErrorID = ?";

static const char STMT_ERROR_NEW[] =
    "INSERT INTO Errors (ProjectID, Hash, Message)"
    " VALUES (?, ?, ?)";

static const char STMT_ERROR_ADD_INPUT[] =
    "INSERT INTO ErrorInputs (ErrorID, InputPath)"
    " VALUES (?, ?)";

static const char STMT_ERROR_ADD_OUTPUT[] =
    "INSERT INTO ErrorOutputs (ErrorID, OutputPath)"
    " VALUES (?, ?)";

static const char STMT_ERROR_QUERY_ALL[] =
    "SELECT ErrorID FROM Errors WHERE ProjectID = ?";

static const char STMT_ERROR_GET_MESSAGE[] =
    "SELECT Message FROM Errors WHERE ErrorID = ?";

ProjectDBConn::ProjectDBConn()
    : m_dbHandle(AssetPipelineOsFuncs::GetPathToProjectDB())

    , m_stmtBeginTransaction(m_dbHandle, STMT_BEGINTRANSAC, sizeof STMT_BEGINTRANSAC)
    , m_stmtEndTransaction(m_dbHandle, STMT_ENDTRANSAC, sizeof STMT_ENDTRANSAC)

    , m_stmtProjectsTable(m_dbHandle, STMT_PROJECTSTABLE, sizeof STMT_PROJECTSTABLE,
                          true)
    , m_stmtConfigTable(m_dbHandle, STMT_CONFIGTABLE, sizeof STMT_CONFIGTABLE,
                        true)
    , m_stmtSetupConfig(m_dbHandle, STMT_SETUPCONFIG, sizeof STMT_SETUPCONFIG,
                        true)
    , m_stmtDepsTable(m_dbHandle, STMT_DEPSTABLE, sizeof STMT_DEPSTABLE, true)
    , m_stmtErrorsTable(m_dbHandle, STMT_ERRORSTABLE, sizeof STMT_ERRORSTABLE, true)
    , m_stmtErrorInputsTable(m_dbHandle, STMT_ERRORINPUTSTABLE,
                             sizeof STMT_ERRORINPUTSTABLE, true)
    , m_stmtErrorOutputsTable(m_dbHandle, STMT_ERROROUTPUTSTABLE,
                              sizeof STMT_ERROROUTPUTSTABLE, true)

    , m_stmtNumProjects(m_dbHandle, STMT_NUMPROJECTS, sizeof STMT_NUMPROJECTS)
    , m_stmtQueryAllProjects(m_dbHandle, STMT_QUERYALLPROJECTS, sizeof STMT_QUERYALLPROJECTS)
    , m_stmtProjectName(m_dbHandle, STMT_PROJECTNAME, sizeof STMT_PROJECTNAME)
    , m_stmtProjectDirectory(m_dbHandle, STMT_PROJECTDIR, sizeof STMT_PROJECTDIR)

    , m_stmtAddProject(m_dbHandle, STMT_ADDPROJECT, sizeof STMT_ADDPROJECT)
    , m_stmtGetActiveProj(m_dbHandle, STMT_GETACTIVEPROJ, sizeof STMT_GETACTIVEPROJ)
    , m_stmtSetActiveProj(m_dbHandle, STMT_SETACTIVEPROJ, sizeof STMT_SETACTIVEPROJ)

    , m_stmtClearDeps(m_dbHandle, STMT_CLEARDEPS, sizeof STMT_CLEARDEPS)
    , m_stmtRecordDep(m_dbHandle, STMT_RECORDDEP, sizeof STMT_RECORDDEP)
    , m_stmtGetDeps(m_dbHandle, STMT_GETDEPS, sizeof STMT_GETDEPS)

    , m_stmtFetchErrors(m_dbHandle, STMT_FETCHERRORS, sizeof STMT_FETCHERRORS)
    , m_stmtErrorGetInputs(m_dbHandle, STMT_ERRORGETINPUTS, sizeof STMT_ERRORGETINPUTS)
    , m_stmtErrorGetOutputs(m_dbHandle, STMT_ERRORGETOUTPUTS, sizeof STMT_ERRORGETOUTPUTS)
    , m_stmtErrorDelete1(m_dbHandle, STMT_ERROR_DELETE1, sizeof STMT_ERROR_DELETE1)
    , m_stmtErrorDelete2(m_dbHandle, STMT_ERROR_DELETE2, sizeof STMT_ERROR_DELETE2)
    , m_stmtErrorDelete3(m_dbHandle, STMT_ERROR_DELETE3, sizeof STMT_ERROR_DELETE3)

    , m_stmtNewError(m_dbHandle, STMT_ERROR_NEW, sizeof STMT_ERROR_NEW)
    , m_stmtErrorAddInput(m_dbHandle, STMT_ERROR_ADD_INPUT, sizeof STMT_ERROR_ADD_INPUT)
    , m_stmtErrorAddOutput(m_dbHandle, STMT_ERROR_ADD_OUTPUT, sizeof STMT_ERROR_ADD_OUTPUT)
    , m_stmtQueryAllErrors(m_dbHandle, STMT_ERROR_QUERY_ALL, sizeof STMT_ERROR_QUERY_ALL)
    , m_stmtErrorGetMessage(m_dbHandle, STMT_ERROR_GET_MESSAGE, sizeof STMT_ERROR_GET_MESSAGE)
{}

unsigned ProjectDBConn::NumProjects() const
{
    if (!m_stmtNumProjects.GetNextRow(m_dbHandle))
        FATAL("No data found");

    int result = m_stmtNumProjects.ColumnInt(0);

    m_stmtNumProjects.Reset(m_dbHandle);

    return (unsigned)result;
}

void ProjectDBConn::QueryAllProjectIDs(std::vector<int>* vec) const
{
    ASSERT(vec);

    vec->clear();

    while (m_stmtQueryAllProjects.GetNextRow(m_dbHandle))
        vec->push_back(m_stmtQueryAllProjects.ColumnInt(0));
}

std::string ProjectDBConn::GetProjectName(int projID) const
{
    ASSERT(projID >= 0);

    m_stmtProjectName.BindInt(1, projID);

    if (!m_stmtProjectName.GetNextRow(m_dbHandle))
        FATAL("No data found");

    std::string ret(m_stmtProjectName.ColumnText(0));

    m_stmtProjectName.Reset(m_dbHandle);

    return ret;
}

std::string ProjectDBConn::GetProjectDirectory(int projID) const
{
    ASSERT(projID >= 0);

    m_stmtProjectDirectory.BindInt(1, projID);

    if (!m_stmtProjectDirectory.GetNextRow(m_dbHandle))
        FATAL("No data found");

    std::string ret(m_stmtProjectDirectory.ColumnText(0));

    m_stmtProjectDirectory.Reset(m_dbHandle);

    return ret;
}

void ProjectDBConn::AddProject(const char* name, const char* directory)
{
    m_stmtAddProject.BindText(1, name);
    m_stmtAddProject.BindText(2, directory);

    m_stmtAddProject.Exec(m_dbHandle);
}

int ProjectDBConn::GetActiveProjectID() const
{
    if (!m_stmtGetActiveProj.GetNextRow(m_dbHandle))
        FATAL("No data found");

    int result;
    if (m_stmtGetActiveProj.IsColumnNull(0)) {
        result = -1;
    } else {
        result = m_stmtGetActiveProj.ColumnInt(0);
    }

    m_stmtGetActiveProj.Reset(m_dbHandle);

    return result;
}

void ProjectDBConn::SetActiveProjectID(int projID)
{
    ASSERT(projID == -1 || projID >= 0);

    if (projID == -1) {
        m_stmtSetActiveProj.BindNull(1);
    } else {
        m_stmtSetActiveProj.BindInt(1, projID);
    }

    m_stmtSetActiveProj.Exec(m_dbHandle);
}

void ProjectDBConn::ClearDependencies(int projID, const char* outputFile)
{
    ASSERT(projID >= 0);
    ASSERT(outputFile);

    m_stmtClearDeps.BindInt(1, projID);
    m_stmtClearDeps.BindText(2, outputFile);

    m_stmtClearDeps.Exec(m_dbHandle);
}

void ProjectDBConn::RecordDependency(int projID, const char* outputFile,
                                     const char* inputFile)
{
    ASSERT(projID >= 0);
    ASSERT(outputFile);
    ASSERT(inputFile);

    m_stmtRecordDep.BindInt(1, projID);
    m_stmtRecordDep.BindText(2, inputFile);
    m_stmtRecordDep.BindText(3, outputFile);

    m_stmtRecordDep.Exec(m_dbHandle);
}

void ProjectDBConn::GetDependents(int projID, const char* inputFile,
                                  std::vector<std::string>* outputFiles)
{
    ASSERT(projID >= 0);
    ASSERT(inputFile);
    ASSERT(outputFiles);

    outputFiles->clear();

    m_stmtGetDeps.BindInt(1, projID);
    m_stmtGetDeps.BindText(2, inputFile);

    while (m_stmtGetDeps.GetNextRow(m_dbHandle))
        outputFiles->push_back(m_stmtGetDeps.ColumnText(0));
}

void ProjectDBConn::ClearError(int projID,
                               const std::vector<std::string>& inputFiles,
                               const std::vector<std::string>& outputFiles)
{
    int errorID = FindErrorID(projID, inputFiles, outputFiles);

    if (errorID == -1)
        return; // Error not in database; don't need to do anything.

    m_stmtErrorDelete1.BindInt(1, errorID);
    m_stmtErrorDelete2.BindInt(1, errorID);
    m_stmtErrorDelete3.BindInt(1, errorID);

    m_stmtBeginTransaction.Exec(m_dbHandle);
    m_stmtErrorDelete1.Exec(m_dbHandle);
    m_stmtErrorDelete2.Exec(m_dbHandle);
    m_stmtErrorDelete3.Exec(m_dbHandle);
    m_stmtEndTransaction.Exec(m_dbHandle);
}

// TODO: This is case sensitive. Is that what we want?
static u64 Hash(const std::vector<std::string>& inputFiles,
                const std::vector<std::string>& outputFiles)
{
    MD5_CTX ctx;
    MD5_Init(&ctx);

    for (size_t i = 0; i < inputFiles.size(); ++i) {
        MD5_Update(&ctx, (void*)inputFiles[i].c_str(), inputFiles[i].size());
    }
    for (size_t i = 0; i < outputFiles.size(); ++i) {
        MD5_Update(&ctx, (void*)outputFiles[i].c_str(), outputFiles[i].size());
    }

    u8 result[16];
    MD5_Final(result, &ctx);
    u64 ret = (u64(result[0]) << 56) | (u64(result[1]) << 48) |
              (u64(result[2]) << 40) | (u64(result[3]) << 32) |
              (u64(result[4]) << 24) | (u64(result[5]) << 16) |
              (u64(result[6]) << 8) | u64(result[7]);

    return ret;
}

void ProjectDBConn::RecordError(int projID,
                                const std::vector<std::string>& inputFiles,
                                const std::vector<std::string>& outputFiles,
                                const std::string& errorMessage)
{
    // TODO: Don't necessarily need to clear the error every time.
    ClearError(projID, inputFiles, outputFiles);

    u64 hash = Hash(inputFiles, outputFiles);

    m_stmtNewError.BindInt(1, projID);
    m_stmtNewError.BindInt64(2, hash);
    m_stmtNewError.BindText(3, errorMessage.c_str());

    m_stmtNewError.Exec(m_dbHandle);

    i64 errorID = m_dbHandle.LastInsertRowID();
    if (errorID > INT_MAX)
        FATAL("Error ID too large");

    for (size_t i = 0; i < inputFiles.size(); ++i) {
        m_stmtErrorAddInput.BindInt(1, (int)errorID);
        m_stmtErrorAddInput.BindText(2, inputFiles[i].c_str());

        m_stmtErrorAddInput.Exec(m_dbHandle);
    }
    for (size_t i = 0; i < outputFiles.size(); ++i) {
        m_stmtErrorAddOutput.BindInt(1, (int)errorID);
        m_stmtErrorAddOutput.BindText(2, outputFiles[i].c_str());

        m_stmtErrorAddOutput.Exec(m_dbHandle);
    }
}

// Returns -1 if not found.
int ProjectDBConn::FindErrorID(
    int projID,
    const std::vector<std::string>& inputFiles,
    const std::vector<std::string>& outputFiles
)
{
    u64 hash = Hash(inputFiles, outputFiles);

    m_stmtFetchErrors.BindInt(1, projID);
    m_stmtFetchErrors.BindInt64(2, hash);

    int trueErrorID = -1;

    while (m_stmtFetchErrors.GetNextRow(m_dbHandle)) {
        int errorID = m_stmtFetchErrors.ColumnInt(0);

        if (MatchError(errorID, inputFiles, outputFiles)) {
            m_stmtFetchErrors.Reset(m_dbHandle);
            trueErrorID = errorID;
            break;
        }
    }

    return trueErrorID;
}

bool ProjectDBConn::MatchError(
    int errorID,
    const std::vector<std::string>& inputFiles,
    const std::vector<std::string>& outputFiles)
{
    return MatchInputsOrOutputs(errorID, inputFiles, m_stmtErrorGetInputs) &&
           MatchInputsOrOutputs(errorID, outputFiles, m_stmtErrorGetOutputs);
}

bool ProjectDBConn::MatchInputsOrOutputs(
    int errorID,
    const std::vector<std::string>& paths,
    SQLiteStatement& stmt
)
{
    stmt.BindInt(1, errorID);

    bool matched = true;

    int i = 0;
    for (; stmt.GetNextRow(m_dbHandle); ++i) {
        const char* path = stmt.ColumnText(0);
        if (strcmp(path, paths[i].c_str()) != 0) {
            stmt.Reset(m_dbHandle);
            matched = false;
            break;
        }
    }

    return matched;
}

void ProjectDBConn::QueryAllErrorIDs(int projID, std::vector<int>* vec) const
{
    ASSERT(vec);
    ASSERT(projID >= 0);

    vec->clear();

    m_stmtQueryAllErrors.BindInt(1, projID);
    while (m_stmtQueryAllErrors.GetNextRow(m_dbHandle))
        vec->push_back(m_stmtQueryAllErrors.ColumnInt(0));
}

std::string ProjectDBConn::GetErrorMessage(int errorID) const
{
    ASSERT(errorID >= 0);

    m_stmtErrorGetMessage.BindInt(1, errorID);
    if (!m_stmtErrorGetMessage.GetNextRow(m_dbHandle))
        FATAL("No data found");

    std::string ret(m_stmtErrorGetMessage.ColumnText(0));

    m_stmtErrorGetMessage.Reset(m_dbHandle);

    return ret;
}

void ProjectDBConn::GetErrorInputPaths(int errorID,
                                       std::vector<std::string>* inputFiles) const
{
    ASSERT(inputFiles);
    ASSERT(errorID >= 0);

    inputFiles->clear();

    m_stmtErrorGetInputs.BindInt(1, errorID);
    while (m_stmtErrorGetInputs.GetNextRow(m_dbHandle))
        inputFiles->push_back(m_stmtErrorGetInputs.ColumnText(0));
}

void ProjectDBConn::GetErrorOutputPaths(int errorID,
                                        std::vector<std::string>* outputFiles) const
{
    ASSERT(outputFiles);
    ASSERT(errorID >= 0);

    outputFiles->clear();

    m_stmtErrorGetOutputs.BindInt(1, errorID);
    while (m_stmtErrorGetOutputs.GetNextRow(m_dbHandle))
        outputFiles->push_back(m_stmtErrorGetOutputs.ColumnText(0));
}
