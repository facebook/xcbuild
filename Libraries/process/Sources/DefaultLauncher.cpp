/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#include <process/DefaultLauncher.h>
#include <process/LaunchResult.h>
#include <libutil/Filesystem.h>

#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

using process::DefaultLauncher;
using process::LaunchResult;
using libutil::Filesystem;

DefaultLauncher::
DefaultLauncher() :
    Launcher()
{
}

DefaultLauncher::
~DefaultLauncher()
{
}

ext::optional<LaunchResult> DefaultLauncher::
launch(Filesystem *filesystem, Context const *context)
{
    /*
     * Extract input data for exec, so no C++ is required after fork.
     */
    std::string path = context->executablePath();
    if (!filesystem->isExecutable(path)) {
        return ext::nullopt;
    }
    char const *cPath = path.c_str();

    std::string directory = context->currentDirectory();
    char const *cDirectory = directory.c_str();

    /* Compute command-line arguments. */
    std::vector<char const *> execArgs;
    execArgs.push_back(cPath);

    std::vector<std::string> arguments = context->commandLineArguments();
    for (std::string const &argument : arguments) {
        execArgs.push_back(argument.c_str());
    }

    execArgs.push_back(nullptr);
    char *const *cExecArgs = const_cast<char *const *>(execArgs.data());

    /* Compute environment variables. */
    std::vector<std::string> envValues;
    for (auto const &value : context->environmentVariables()) {
        envValues.push_back(value.first + "=" + value.second);
    }

    std::vector<char const *> execEnv;
    for (auto const &value : envValues) {
        execEnv.push_back(value.c_str());
    }
    execEnv.push_back(nullptr);
    char *const *cExecEnv = const_cast<char *const *>(execEnv.data());

    /* Compute user. */
    uid_t uid = context->userID();
    gid_t gid = context->groupID();

    /* Prepare pipes for forked process's stdout and stderr output. */
    const int readEnd = 0;
    const int writeEnd = 1;
    int stdoutPipe[2];
    int stderrPipe[2];
    if (::pipe(stdoutPipe) == -1 || ::pipe(stderrPipe) == -1) {
        /* Could not create pipes. */
        return ext::nullopt;
    }

    /*
     * Fork new process.
     */
    pid_t pid = fork();
    if (pid < 0) {
        /* Fork failed. */
        return ext::nullopt;
    } else if (pid == 0) {
        /* Fork succeeded, new process. */
        if (::chdir(cDirectory) == -1) {
            ::perror("chdir");
            ::_exit(1);
        }

        if (::setuid(uid) == -1) {
            ::perror("setuid");
            ::_exit(1);
        }

        if (::setgid(gid) == -1) {
            ::perror("setgid");
            ::_exit(1);
        }

        /* Redirect stdout and stderr. */
        ::dup2(stdoutPipe[writeEnd], STDOUT_FILENO);
        ::close(stdoutPipe[readEnd]);
        ::close(stderrPipe[writeEnd]);
        ::dup2(stderrPipe[writeEnd], STDERR_FILENO);
        ::close(stderrPipe[readEnd]);
        ::close(stderrPipe[writeEnd]);

        ::execve(cPath, cExecArgs, cExecEnv);
        ::_exit(-1);

        return ext::nullopt;
    } else {
        /* Fork succeeded, existing process. */
        int status;
        ::waitpid(pid, &status, 0);

        /* Read stdout and stderr. */
        ::close(stdoutPipe[writeEnd]);
        ::close(stderrPipe[writeEnd]);

        std::string standardOutput = "";
        char stdoutBuffer[32];
        while (ssize_t bytes = ::read(stdoutPipe[readEnd], stdoutBuffer, sizeof(stdoutBuffer))) {
            standardOutput.append(stdoutBuffer, bytes);
            ::memset(stdoutBuffer, 0, sizeof(stdoutBuffer));
        }
        ::close(stdoutPipe[readEnd]);

        std::string standardError = "";
        char stderrBuffer[32];
        while (ssize_t bytes = ::read(stderrPipe[readEnd], stderrBuffer, sizeof(stderrBuffer))) {
            standardError.append(stderrBuffer, bytes);
            ::memset(stderrBuffer, 0, sizeof(stderrBuffer));
        }
        ::close(stderrPipe[readEnd]);

        return LaunchResult(WEXITSTATUS(status), standardOutput, standardError);
    }
}
