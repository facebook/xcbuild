/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#include <process/DefaultContext.h>
#include <libutil/FSUtil.h>

#include <mutex>
#include <sstream>
#include <unordered_set>
#include <cstring>
#include <cassert>

#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <linux/limits.h>
#if __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 16
#include <sys/auxv.h>
#endif
#endif

extern "C" char **environ;

using process::DefaultContext;
using libutil::FSUtil;

DefaultContext::
DefaultContext() :
    Context()
{
}

DefaultContext::
~DefaultContext()
{
}

std::string const &DefaultContext::
currentDirectory() const
{
    static std::string const *directory = nullptr;

    std::once_flag flag;
    std::call_once(flag, []{
        char path[PATH_MAX + 1];
        if (::getcwd(path, sizeof(path)) == nullptr) {
            path[0] = '\0';
        }

        directory = new std::string(path);
    });

    return *directory;
}

#if defined(__linux__)
static char initialWorkingDirectory[PATH_MAX] = { 0 };
__attribute__((constructor))
static void InitializeInitialWorkingDirectory()
{
    if (getcwd(initialWorkingDirectory, sizeof(initialWorkingDirectory)) == NULL) {
        abort();
    }
}

#if !(__GLIBC__ >= 2 && __GLIBC_MINOR__ >= 16)
static char initialExecutablePath[PATH_MAX] = { 0 };
__attribute__((constructor))
static void InitialExecutablePathInitialize(int argc, char **argv)
{
    strncpy(initialExecutablePath, argv[0], sizeof(initialExecutablePath));
}
#endif
#endif

std::string const &DefaultContext::
executablePath() const
{
    static std::string const *executablePath = nullptr;

    std::once_flag flag;
    std::call_once(flag, []{
#if defined(__APPLE__)
        uint32_t size = 0;
        if (_NSGetExecutablePath(NULL, &size) != -1) {
            abort();
        }

        std::string absolutePath;
        absolutePath.resize(size);
        if (_NSGetExecutablePath(&absolutePath[0], &size) != 0) {
            abort();
        }
#elif defined(__linux__)
#if __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 16
        char const *path = reinterpret_cast<char const *>(getauxval(AT_EXECFN));
        if (path == NULL) {
            abort();
        }
#elif defined(__GLIBC__)
        char const *path = reinterpret_cast<char const *>(initialExecutablePath);
#else
#error Requires glibc on Linux.
#endif
        std::string absolutePath = FSUtil::ResolveRelativePath(std::string(path), std::string(initialWorkingDirectory));
#else
#error Unsupported platform.
#endif

        executablePath = new std::string(FSUtil::NormalizePath(absolutePath));
    });

    return *executablePath;
}

static int commandLineArgumentCount = 0;
static char **commandLineArgumentValues = NULL;

#if !defined(__linux__)
__attribute__((constructor))
#endif
static void CommandLineArgumentsInitialize(int argc, char **argv)
{
    commandLineArgumentCount = argc;
    commandLineArgumentValues = argv;
}

#if defined(__linux__)
__attribute__((section(".init_array"))) auto commandLineArgumentInitializer = &CommandLineArgumentsInitialize;
#endif

std::vector<std::string> const &DefaultContext::
commandLineArguments() const
{
    static std::vector<std::string> const *arguments = nullptr;

    std::once_flag flag;
    std::call_once(flag, []{
        arguments = new std::vector<std::string>(commandLineArgumentValues + 1, commandLineArgumentValues + commandLineArgumentCount);
    });

    return *arguments;
}

ext::optional<std::string> DefaultContext::
environmentVariable(std::string const &variable) const
{
    if (char *value = getenv(variable.c_str())) {
        return std::string(value);
    } else {
        return ext::nullopt;
    }
}

std::unordered_map<std::string, std::string> const &DefaultContext::
environmentVariables() const
{
    static std::unordered_map<std::string, std::string> const *environment = nullptr;

    std::once_flag flag;
    std::call_once(flag, []{
        std::unordered_map<std::string, std::string> values;

        for (char **current = environ; *current; current++) {
            std::string variable = *current;
            std::string::size_type offset = variable.find('=');

            std::string name = variable.substr(0, offset);
            std::string value = variable.substr(offset + 1);
            values.insert({ name, value });
        }

        environment = new std::unordered_map<std::string, std::string>(values);
    });

    return *environment;
}

std::string const &DefaultContext::
userName() const
{
    static std::string const *userName = nullptr;

    std::once_flag flag;
    std::call_once(flag, []{
        if (struct passwd const *pw = ::getpwuid(::getuid())) {
            if (pw->pw_name != nullptr) {
                userName = new std::string(pw->pw_name);
            }
        }

        if (userName == nullptr) {
            std::ostringstream os;
            os << ::getuid();
            userName = new std::string(os.str());
        }
    });

    return *userName;
}

std::string const &DefaultContext::
groupName() const
{
    static std::string const *groupName = nullptr;

    std::once_flag flag;
    std::call_once(flag, []{
        if (struct group const *gr = ::getgrgid(::getgid())) {
            if (gr->gr_name != nullptr) {
                groupName = new std::string(gr->gr_name);
            }
        }

        if (groupName == nullptr) {
            std::ostringstream os;
            os << ::getgid();
            groupName = new std::string(os.str());
        }
    });

    return *groupName;
}

int32_t DefaultContext::
userID() const
{
    return ::getuid();
}

int32_t DefaultContext::
groupID() const
{
    return ::getgid();
}

ext::optional<std::string> DefaultContext::
userHomeDirectory() const
{
    if (ext::optional<std::string> value = Context::userHomeDirectory()) {
        return value;
    } else {
        char *home = ::getpwuid(::getuid())->pw_dir;
        return std::string(home);
    }
}
