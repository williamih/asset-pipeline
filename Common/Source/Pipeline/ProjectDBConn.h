#ifndef PIPELINE_PROJECTDBCONN_H
#define PIPELINE_PROJECTDBCONN_H

#include <string>
#include <vector>
#include <Core/Types.h>

struct sqlite3;
struct sqlite3_stmt;

class ProjectDBConn {
public:
    ProjectDBConn();

    unsigned NumProjects() const;
    void QueryAllProjectIDs(std::vector<int>* vec) const;

    std::string GetProjectName(int projID) const;
    std::string GetProjectDirectory(int projID) const;

    void AddProject(const char* name, const char* directory);

    // Returns -1 if the active project is null.
    int GetActiveProjectID() const;
    void SetActiveProjectID(int projID);

    void ClearDependencies(int projID, const char* outputFile);
    void RecordDependency(int projID, const char* outputFile,
                          const char* inputFile);
    void GetDependents(int projID, const char* inputFile,
                       std::vector<std::string>* outputFiles);

    void ClearError(int projID,
                    const std::vector<std::string>& inputFiles,
                    const std::vector<std::string>& outputFiles);
    void RecordError(int projID,
                     const std::vector<std::string>& inputFiles,
                     const std::vector<std::string>& outputFiles,
                     const std::string& errorMessage);
    void QueryAllErrorIDs(int projID, std::vector<int>* vec) const;
    std::string GetErrorMessage(int errorID) const;
    void GetErrorInputPaths(int errorID, std::vector<std::string>* inputFiles) const;
    void GetErrorOutputPaths(int errorID, std::vector<std::string>* outputFiles) const;

private:
    class SQLiteStatement;

    struct DBHandle {
        explicit DBHandle(const std::string& path);
        ~DBHandle();

        i64 LastInsertRowID() const;

    private:
        DBHandle(const DBHandle&);
        DBHandle& operator=(const DBHandle&);

        friend class SQLiteStatement;

        sqlite3* db;
    };

    class SQLiteStatement {
    public:
        SQLiteStatement(DBHandle& db, const char* text, int nBytes,
                        bool exec = false);
        ~SQLiteStatement();

        void Exec(const DBHandle& db);
        bool GetNextRow(const DBHandle& db);
        void Reset(const DBHandle& db);
        void BindNull(int pos);
        void BindInt(int pos, int value);
        void BindInt64(int pos, i64 value);
        void BindText(int pos, const char* str);
        int ColumnInt(int index);
        const char* ColumnText(int index);
        bool IsColumnNull(int index);

    private:
        SQLiteStatement(const SQLiteStatement&);
        SQLiteStatement& operator=(const SQLiteStatement&);

        sqlite3_stmt* stmt;
    };

    ProjectDBConn(const ProjectDBConn&);
    ProjectDBConn& operator=(const ProjectDBConn&);

    int FindErrorID(
        int projID,
        const std::vector<std::string>& inputFiles,
        const std::vector<std::string>& outputFiles
    );
    bool MatchError(
        int errorID,
        const std::vector<std::string>& inputFiles,
        const std::vector<std::string>& outputFiles
    );
    bool MatchInputsOrOutputs(
        int errorID,
        const std::vector<std::string>& paths,
        SQLiteStatement& stmt
    );

    DBHandle m_dbHandle;

    SQLiteStatement m_stmtBeginTransaction;
    SQLiteStatement m_stmtEndTransaction;

    SQLiteStatement m_stmtProjectsTable;
    SQLiteStatement m_stmtConfigTable;
    SQLiteStatement m_stmtSetupConfig;
    SQLiteStatement m_stmtDepsTable;
    SQLiteStatement m_stmtErrorsTable;
    SQLiteStatement m_stmtErrorInputsTable;
    SQLiteStatement m_stmtErrorOutputsTable;

    mutable SQLiteStatement m_stmtNumProjects;
    mutable SQLiteStatement m_stmtQueryAllProjects;
    mutable SQLiteStatement m_stmtProjectName;
    mutable SQLiteStatement m_stmtProjectDirectory;

    SQLiteStatement m_stmtAddProject;
    mutable SQLiteStatement m_stmtGetActiveProj;
    SQLiteStatement m_stmtSetActiveProj;

    SQLiteStatement m_stmtClearDeps;
    SQLiteStatement m_stmtRecordDep;
    SQLiteStatement m_stmtGetDeps;

    SQLiteStatement m_stmtFetchErrors;
    mutable SQLiteStatement m_stmtErrorGetInputs;
    mutable SQLiteStatement m_stmtErrorGetOutputs;
    SQLiteStatement m_stmtErrorDelete1;
    SQLiteStatement m_stmtErrorDelete2;
    SQLiteStatement m_stmtErrorDelete3;
    SQLiteStatement m_stmtNewError;
    SQLiteStatement m_stmtErrorAddInput;
    SQLiteStatement m_stmtErrorAddOutput;
    mutable SQLiteStatement m_stmtQueryAllErrors;
    mutable SQLiteStatement m_stmtErrorGetMessage;
};

#endif // PIPELINE_PROJECTDBCONN_H
