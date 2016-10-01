#ifndef PIPELINE_PROCESS_H
#define PIPELINE_PROCESS_H

#include <vector>
#include <string>

enum ProcessCreationResult {
    PROCESS_SUCCESS,
    PROCESS_NOT_FOUND
};

struct Process {
    Process(const char* path, const std::vector<const char*>& args);

    ProcessCreationResult result;
    int status;
    std::string stdoutStr;
    std::string stderrStr;
};

#endif // PIPELINE_PROCESS_H
