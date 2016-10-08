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

ProjectDBConn::SQLiteStatement::SQLiteStatement(sqlite3* db, const char* text,
                                                int nBytes, bool exec)
    : stmt(NULL)
{
    if (sqlite3_prepare_v2(db, text, nBytes, &stmt, NULL) != SQLITE_OK) {
        FATAL("sqlite3_prepare_v2: %s", sqlite3_errmsg(db));
    }
    if (exec) {
        if (sqlite3_step(stmt) != SQLITE_DONE)
            FATAL("sqlite3_step: %s", sqlite3_errmsg(db));
        if (sqlite3_reset(stmt) != SQLITE_OK)
            FATAL("sqlite_reset: %s", sqlite3_errmsg(db));
    }
}

ProjectDBConn::SQLiteStatement::~SQLiteStatement()
{
    if (sqlite3_finalize(stmt) != SQLITE_OK)
        FATAL("sqlite3_finalize");
}

int ProjectDBConn::SQLiteStatement::Step(sqlite3* db)
{
    int ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE && ret != SQLITE_BUSY && ret != SQLITE_ROW)
        FATAL("sqlite3_step: %s", sqlite3_errmsg(db));
    return ret;
}

void ProjectDBConn::SQLiteStatement::Reset(sqlite3* db)
{
    if (sqlite3_reset(stmt) != SQLITE_OK)
        FATAL("sqlite3_reset: %s", sqlite3_errmsg(db));
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
{
    m_stmtProjectsTable.Reset(m_dbHandle);
    m_stmtConfigTable.Reset(m_dbHandle);
    m_stmtSetupConfig.Reset(m_dbHandle);
    m_stmtDepsTable.Reset(m_dbHandle);
    m_stmtErrorsTable.Reset(m_dbHandle);
    m_stmtErrorInputsTable.Reset(m_dbHandle);
    m_stmtErrorOutputsTable.Reset(m_dbHandle);
}

unsigned ProjectDBConn::NumProjects() const
{
    if (m_stmtNumProjects.Step(m_dbHandle) != SQLITE_ROW)
        FATAL("sqlite3_step: %s", sqlite3_errmsg(m_dbHandle));

    int result = sqlite3_column_int(m_stmtNumProjects, 0);

    m_stmtNumProjects.Reset(m_dbHandle);

    return (unsigned)result;
}

void ProjectDBConn::QueryAllProjectIDs(std::vector<int>* vec) const
{
    ASSERT(vec);

    vec->clear();

    int ret;
    while ((ret = m_stmtQueryAllProjects.Step(m_dbHandle)) == SQLITE_ROW) {
        int projID = sqlite3_column_int(m_stmtQueryAllProjects, 0);
        vec->push_back(projID);
    }
    if (ret != SQLITE_DONE)
        FATAL("sqlite3_step: %s", sqlite3_errmsg(m_dbHandle));

    m_stmtQueryAllProjects.Reset(m_dbHandle);
}

std::string ProjectDBConn::GetProjectName(int projID) const
{
    ASSERT(projID >= 0);

    if (sqlite3_bind_int(m_stmtProjectName, 1, projID) != SQLITE_OK)
        FATAL("sqlite3_bind_int");

    if (m_stmtProjectName.Step(m_dbHandle) != SQLITE_ROW)
        FATAL("sqlite3_step: %s", sqlite3_errmsg(m_dbHandle));

    const unsigned char* result = sqlite3_column_text(m_stmtProjectName, 0);
    std::string ret((const char*)result);

    m_stmtProjectName.Reset(m_dbHandle);

    return ret;
}

std::string ProjectDBConn::GetProjectDirectory(int projID) const
{
    ASSERT(projID >= 0);

    if (sqlite3_bind_int(m_stmtProjectDirectory, 1, projID) != SQLITE_OK)
        FATAL("sqlite3_bind_int");

    if (m_stmtProjectDirectory.Step(m_dbHandle) != SQLITE_ROW)
        FATAL("sqlite3_step: %s", sqlite3_errmsg(m_dbHandle));

    const unsigned char* result = sqlite3_column_text(m_stmtProjectDirectory, 0);
    std::string ret((const char*)result);

    m_stmtProjectDirectory.Reset(m_dbHandle);

    return ret;
}

void ProjectDBConn::AddProject(const char* name, const char* directory)
{
    char* nameCopy = strdup(name);
    char* dirCopy = strdup(directory);

    if (sqlite3_bind_text(m_stmtAddProject, 1, nameCopy, -1, free) != SQLITE_OK)
        FATAL("sqlite3_bind_text");

    if (sqlite3_bind_text(m_stmtAddProject, 2, dirCopy, -1, free) != SQLITE_OK)
        FATAL("sqlite3_bind_text");

    m_stmtAddProject.Step(m_dbHandle);
    m_stmtAddProject.Reset(m_dbHandle);
}

int ProjectDBConn::GetActiveProjectID() const
{
    if (m_stmtGetActiveProj.Step(m_dbHandle) != SQLITE_ROW)
        FATAL("sqlite3_step: %s", sqlite3_errmsg(m_dbHandle));

    int result;
    if (sqlite3_column_type(m_stmtGetActiveProj, 0) == SQLITE_NULL) {
        result = -1;
    } else {
        result = sqlite3_column_int(m_stmtGetActiveProj, 0);
    }

    m_stmtGetActiveProj.Reset(m_dbHandle);

    return result;
}

void ProjectDBConn::SetActiveProjectID(int projID)
{
    ASSERT(projID == -1 || projID >= 0);

    if (projID == -1) {
        if (sqlite3_bind_null(m_stmtSetActiveProj, 1) != SQLITE_OK)
            FATAL("sqlite3_bind_null");
    } else {
        if (sqlite3_bind_int(m_stmtSetActiveProj, 1, projID) != SQLITE_OK)
            FATAL("sqlite3_bind_int");
    }

    if (m_stmtSetActiveProj.Step(m_dbHandle) != SQLITE_DONE)
        FATAL("sqlite3_step: %s", sqlite3_errmsg(m_dbHandle));

    m_stmtSetActiveProj.Reset(m_dbHandle);
}

void ProjectDBConn::ClearDependencies(int projID, const char* outputFile)
{
    ASSERT(projID >= 0);
    ASSERT(outputFile);

    if (sqlite3_bind_int(m_stmtClearDeps, 1, projID) != SQLITE_OK)
        FATAL("sqlite3_bind_int");

    if (sqlite3_bind_text(m_stmtClearDeps, 2, strdup(outputFile), -1, free) != SQLITE_OK)
        FATAL("sqlite3_bind_text");

    if (m_stmtClearDeps.Step(m_dbHandle) != SQLITE_DONE)
        FATAL("sqlite3_step: %s", sqlite3_errmsg(m_dbHandle));

    m_stmtClearDeps.Reset(m_dbHandle);
}

void ProjectDBConn::RecordDependency(int projID, const char* outputFile,
                                     const char* inputFile)
{
    ASSERT(projID >= 0);
    ASSERT(outputFile);
    ASSERT(inputFile);

    if (sqlite3_bind_int(m_stmtRecordDep, 1, projID) != SQLITE_OK)
        FATAL("sqlite3_bind_int");

    if (sqlite3_bind_text(m_stmtRecordDep, 2, strdup(inputFile), -1, free) != SQLITE_OK)
        FATAL("sqlite3_bind_text");

    if (sqlite3_bind_text(m_stmtRecordDep, 3, strdup(outputFile), -1, free) != SQLITE_OK)
        FATAL("sqlite3_bind_text");

    if (m_stmtRecordDep.Step(m_dbHandle) != SQLITE_DONE)
        FATAL("sqlite3_step: %s", sqlite3_errmsg(m_dbHandle));

    m_stmtRecordDep.Reset(m_dbHandle);
}

void ProjectDBConn::GetDependents(int projID, const char* inputFile,
                                  std::vector<std::string>* outputFiles)
{
    ASSERT(projID >= 0);
    ASSERT(inputFile);
    ASSERT(outputFiles);

    outputFiles->clear();

    if (sqlite3_bind_int(m_stmtGetDeps, 1, projID) != SQLITE_OK)
        FATAL("sqlite3_bind_int");

    if (sqlite3_bind_text(m_stmtGetDeps, 2, strdup(inputFile), -1, free) != SQLITE_OK)
        FATAL("sqlite3_bind_text");

    int ret;
    while ((ret = m_stmtGetDeps.Step(m_dbHandle)) == SQLITE_ROW) {
        const char* str = (const char*)sqlite3_column_text(m_stmtGetDeps, 0);
        outputFiles->push_back(str);
    }
    if (ret != SQLITE_DONE)
        FATAL("sqlite3_step: %s", sqlite3_errmsg(m_dbHandle));

    m_stmtGetDeps.Reset(m_dbHandle);
}

void ProjectDBConn::ClearError(int projID,
                               const std::vector<std::string>& inputFiles,
                               const std::vector<std::string>& outputFiles)
{
    int errorID = FindErrorID(projID, inputFiles, outputFiles);

    if (errorID == -1)
        return; // Error not in database; don't need to do anything.

    if (sqlite3_bind_int(m_stmtErrorDelete1, 1, errorID) != SQLITE_OK)
        FATAL("sqlite3_bind_int");
    if (sqlite3_bind_int(m_stmtErrorDelete2, 1, errorID) != SQLITE_OK)
        FATAL("sqlite3_bind_int");
    if (sqlite3_bind_int(m_stmtErrorDelete3, 1, errorID) != SQLITE_OK)
        FATAL("sqlite3_bind_int");

    if (m_stmtBeginTransaction.Step(m_dbHandle) != SQLITE_DONE) FATAL("sqlite3_step");
    if (m_stmtErrorDelete1.Step(m_dbHandle) != SQLITE_DONE) FATAL("sqlite3_step");
    if (m_stmtErrorDelete2.Step(m_dbHandle) != SQLITE_DONE) FATAL("sqlite3_step");
    if (m_stmtErrorDelete3.Step(m_dbHandle) != SQLITE_DONE) FATAL("sqlite3_step");
    if (m_stmtEndTransaction.Step(m_dbHandle) != SQLITE_DONE) FATAL("sqlite3_step");

    m_stmtBeginTransaction.Reset(m_dbHandle);
    m_stmtErrorDelete1.Reset(m_dbHandle);
    m_stmtErrorDelete2.Reset(m_dbHandle);
    m_stmtErrorDelete3.Reset(m_dbHandle);
    m_stmtEndTransaction.Reset(m_dbHandle);
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

    if (sqlite3_bind_int(m_stmtNewError, 1, projID) != SQLITE_OK)
        FATAL("sqlite3_bind_int");

    if (sqlite3_bind_int64(m_stmtNewError, 2, hash) != SQLITE_OK)
        FATAL("sqlite3_bind_int64");

    if (sqlite3_bind_text(m_stmtNewError, 3, strdup(errorMessage.c_str()),
                          -1, free) != SQLITE_OK)
        FATAL("sqlite_bind_text");

    if (m_stmtNewError.Step(m_dbHandle) != SQLITE_DONE)
        FATAL("sqlite3_step");

    m_stmtNewError.Reset(m_dbHandle);

    sqlite3_int64 errorID = sqlite3_last_insert_rowid(m_dbHandle);
    if (errorID > INT_MAX)
        FATAL("Error ID too large");

    for (size_t i = 0; i < inputFiles.size(); ++i) {
        if (sqlite3_bind_int(m_stmtErrorAddInput, 1, (int)errorID))
            FATAL("sqlite3_bind_int");
        if (sqlite3_bind_text(m_stmtErrorAddInput, 2, strdup(inputFiles[i].c_str()),
                              -1, free) != SQLITE_OK)
            FATAL("sqlite_bind_text");
        if (m_stmtErrorAddInput.Step(m_dbHandle) != SQLITE_DONE)
            FATAL("sqlite3_step");
        m_stmtErrorAddInput.Reset(m_dbHandle);
    }
    for (size_t i = 0; i < outputFiles.size(); ++i) {
        if (sqlite3_bind_int(m_stmtErrorAddOutput, 1, (int)errorID))
            FATAL("sqlite3_bind_int");
        if (sqlite3_bind_text(m_stmtErrorAddOutput, 2, strdup(outputFiles[i].c_str()),
                              -1, free) != SQLITE_OK)
            FATAL("sqlite_bind_text");
        if (m_stmtErrorAddOutput.Step(m_dbHandle) != SQLITE_DONE)
            FATAL("sqlite3_step");
        m_stmtErrorAddOutput.Reset(m_dbHandle);
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

    if (sqlite3_bind_int(m_stmtFetchErrors, 1, projID) != SQLITE_OK)
        FATAL("sqlite3_bind_int");

    if (sqlite3_bind_int64(m_stmtFetchErrors, 2, hash) != SQLITE_OK)
        FATAL("sqlite3_bind_int64");

    int trueErrorID = -1;

    int ret;
    while ((ret = m_stmtFetchErrors.Step(m_dbHandle)) == SQLITE_ROW) {
        int errorID = sqlite3_column_int(m_stmtFetchErrors, 0);

        if (MatchError(errorID, inputFiles, outputFiles)) {
            trueErrorID = errorID;
            break;
        }
    }
    if (ret != SQLITE_DONE && ret != SQLITE_ROW)
        FATAL("sqlite3_step: %s", sqlite3_errmsg(m_dbHandle));
    m_stmtFetchErrors.Reset(m_dbHandle);

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
    if (sqlite3_bind_int(stmt, 1, errorID) != SQLITE_OK)
        FATAL("sqlite3_bind_int");

    bool matched = true;

    int i = 0;
    int ret;
    for (; (ret = stmt.Step(m_dbHandle)) == SQLITE_ROW; ++i) {
        const char* path = (const char*)sqlite3_column_text(stmt, 0);
        if (strcmp(path, paths[i].c_str()) != 0) {
            matched = false;
            break;
        }
    }
    if (ret != SQLITE_DONE && ret != SQLITE_ROW)
        FATAL("sqlite3_step: %s", sqlite3_errmsg(m_dbHandle));
    stmt.Reset(m_dbHandle);

    return matched;
}
