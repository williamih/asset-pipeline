#ifndef PIPELINE_PROJECTDBCONN_H
#define PIPELINE_PROJECTDBCONN_H

#include <string>

struct sqlite3;
struct sqlite3_stmt;

class ProjectDBConn {
public:
    ProjectDBConn();

    unsigned NumProjects() const;
    std::string GetProjectName(unsigned index) const;
    std::string GetProjectDirectory(unsigned index) const;

    void AddProject(const char* name, const char* directory);

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
    mutable SQLiteStatement m_stmtProjectsTable;
    mutable SQLiteStatement m_stmtNumProjects;
    mutable SQLiteStatement m_stmtProjectName;
    mutable SQLiteStatement m_stmtProjectDirectory;
    mutable SQLiteStatement m_stmtAddProject;
};

#endif // PIPELINE_PROJECTDBCONN_H
