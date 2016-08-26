#include "Process.h"

#include <unistd.h>
#include <errno.h>
#include <spawn.h>
#include <poll.h>

#include <Core/Macros.h>

static void ReadPipes(int stdoutReadPipe, int stderrReadPipe,
                      std::string& stdoutStr, std::string& stderrStr)
{
    const unsigned OUTPUT_BUFFER_SIZE_BYTES = 1024;

    std::vector<char> buffer;
    buffer.resize(OUTPUT_BUFFER_SIZE_BYTES);

    pollfd fds[] = { {stdoutReadPipe, POLLIN}, {stderrReadPipe, POLLIN} };
    const int NFDS = sizeof fds / sizeof fds[0];

    for (int rval; (rval = poll(fds, NFDS, (int)buffer.size())) > 0; ) {
        bool eof = false;
        if (fds[0].revents & POLLIN) {
            ssize_t bytesRead = read(stdoutReadPipe, &buffer[0],
                                     buffer.size() - 1);
            if (bytesRead < 0)
                FATAL("read");
            if (bytesRead > 0) {
                buffer[bytesRead] = 0;
                stdoutStr.reserve((size_t)bytesRead);
                stdoutStr.append(&buffer[0]);
            } else {
                eof = true;
            }
        }
        if (fds[1].revents & POLLIN) {
            ssize_t bytesRead = read(stderrReadPipe, &buffer[0],
                                     buffer.size() - 1);
            if (bytesRead < 0)
                FATAL("read");
            if (bytesRead > 0) {
                buffer[bytesRead] = 0;
                stderrStr.reserve((size_t)bytesRead);
                stderrStr.append(&buffer[0]);
                eof = false;
            }
        }
        if (eof)
            break;
    }
}

Process::Process(const char* path, const std::vector<const char*>& args)
    : result(PROCESS_SUCCESS)
    , status(-1)
    , stdoutStr()
    , stderrStr()
{
    if (args.back() != NULL)
        FATAL("Last member of args vector should be a null pointer");

    int stdoutPipe[2];
    int stderrPipe[2];
    posix_spawn_file_actions_t actions;

    if (pipe(stdoutPipe) || pipe(stderrPipe))
        FATAL("pipe");

    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_addclose(&actions, stdoutPipe[0]);
    posix_spawn_file_actions_addclose(&actions, stderrPipe[0]);
    posix_spawn_file_actions_adddup2(&actions, stdoutPipe[1], 1);
    posix_spawn_file_actions_adddup2(&actions, stderrPipe[1], 2);

    posix_spawn_file_actions_addclose(&actions, stdoutPipe[1]);
    posix_spawn_file_actions_addclose(&actions, stderrPipe[1]);

    pid_t pid;
    int spawnResult = posix_spawn(&pid, path, &actions, NULL,
                                  (char* const*)&args[0], NULL);
    if (spawnResult != 0) {
        if (spawnResult == ENOENT || spawnResult == ESRCH) {
            result = PROCESS_NOT_FOUND;
        } else {
            FATAL("Couldn't posix_spawn: %s", strerror(errno));
        }
    }

    close(stdoutPipe[1]);
    close(stderrPipe[1]);

    stdoutStr.clear();
    stderrStr.clear();

    if (spawnResult == 0) {
        ReadPipes(stdoutPipe[0], stderrPipe[0], stdoutStr, stderrStr);

        if (wait(&status) == -1)
            FATAL("wait");
    }

    posix_spawn_file_actions_destroy(&actions);
}
