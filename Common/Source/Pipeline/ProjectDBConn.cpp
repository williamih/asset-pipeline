#include "ProjectDBConn.h"

#include <string.h>
#include <sqlite3/sqlite3.h>
#include <Core/Macros.h>

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

ProjectDBConn::ProjectDBConn()
    : m_dbHandle(AssetPipelineOsFuncs::GetPathToProjectDB())
    , m_stmtProjectsTable(m_dbHandle, STMT_PROJECTSTABLE, sizeof STMT_PROJECTSTABLE,
                          true)
    , m_stmtConfigTable(m_dbHandle, STMT_CONFIGTABLE, sizeof STMT_CONFIGTABLE,
                        true)
    , m_stmtSetupConfig(m_dbHandle, STMT_SETUPCONFIG, sizeof STMT_SETUPCONFIG,
                        true)
    , m_stmtDepsTable(m_dbHandle, STMT_DEPSTABLE, sizeof STMT_DEPSTABLE, true)
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
{
    m_stmtProjectsTable.Reset(m_dbHandle);
    m_stmtConfigTable.Reset(m_dbHandle);
    m_stmtSetupConfig.Reset(m_dbHandle);
    m_stmtDepsTable.Reset(m_dbHandle);
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
