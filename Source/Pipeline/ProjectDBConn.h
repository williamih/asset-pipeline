#ifndef PIPELINE_PROJECTDBCONN_H
#define PIPELINE_PROJECTDBCONN_H

#include <string>
#include <vector>

struct sqlite3;
struct sqlite3_stmt;

class ProjectDBConn {
public:
    ProjectDBConn();

    unsigned NumProjects() const;
    std::string GetProjectName(unsigned index) const;
    std::string GetProjectDirectory(unsigned index) const;

    void AddProject(const char* name, const char* directory);

    // Returns -1 if the active project is null.
    int GetActiveProjectIndex() const;
    void SetActiveProjectIndex(int index);

    void ClearDependencies(unsigned projIdx, const char* outputFile);
    void RecordDependency(unsigned projIdx, const char* outputFile,
                          const char* inputFile);
    void GetDependents(unsigned projIdx, const char* inputFile,
                       std::vector<std::string>* outputFiles);

private:
    struct DBHandle {
        explicit DBHandle(const std::string& path);
        ~DBHandle();

        operator sqlite3*() const { return db; }

        sqlite3* db;

    private:
        DBHandle(const DBHandle&);
        DBHandle& operator=(const DBHandle&);
    };

    struct SQLiteStatement {
        SQLiteStatement(sqlite3* db, const char* text, int nBytes, bool exec = false);
        ~SQLiteStatement();

        operator sqlite3_stmt*() const { return stmt; }

        int Step(sqlite3* db);
        void Reset(sqlite3* db);

        sqlite3_stmt* stmt;

    private:
        SQLiteStatement(const SQLiteStatement&);
        SQLiteStatement& operator=(const SQLiteStatement&);
    };

    ProjectDBConn(const ProjectDBConn&);
    ProjectDBConn& operator=(const ProjectDBConn&);

    DBHandle m_dbHandle;
    SQLiteStatement m_stmtProjectsTable;
    SQLiteStatement m_stmtConfigTable;
    SQLiteStatement m_stmtSetupConfig;
    SQLiteStatement m_stmtDepsTable;
    mutable SQLiteStatement m_stmtNumProjects;
    mutable SQLiteStatement m_stmtProjectName;
    mutable SQLiteStatement m_stmtProjectDirectory;
    SQLiteStatement m_stmtAddProject;
    mutable SQLiteStatement m_stmtGetActiveProj;
    SQLiteStatement m_stmtSetActiveProj;
    SQLiteStatement m_stmtClearDeps;
    SQLiteStatement m_stmtRecordDep;
    SQLiteStatement m_stmtGetDeps;
};

#endif // PIPELINE_PROJECTDBCONN_H
