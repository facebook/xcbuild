/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef __libutil_MemoryProcessContext_h
#define __libutil_MemoryProcessContext_h

#include <libutil/ProcessContext.h>

namespace libutil {

/*
 * A process context with arbitrary values.
 */
class MemoryProcessContext : public ProcessContext {
private:
    std::string _executablePath;
    std::string _currentDirectory;

private:
    std::vector<std::string> _commandLineArguments;
    std::unordered_map<std::string, std::string> _environmentVariables;

private:
    int32_t     _userID;
    int32_t     _groupID;
    std::string _userName;
    std::string _groupName;

public:
    MemoryProcessContext(
        std::string const &executablePath,
        std::string const &currentDirectory,
        std::vector<std::string> const &commandLineArguments,
        std::unordered_map<std::string, std::string> const &environmentVariables,
        int32_t userID,
        int32_t groupID,
        std::string const &userName,
        std::string const &groupName);
    explicit MemoryProcessContext(ProcessContext const *processContext);
    virtual ~MemoryProcessContext();

public:
    virtual std::string executablePath() const
    { return _executablePath; }
    std::string &executablePath()
    { return _executablePath; }

    virtual std::string currentDirectory() const
    { return _currentDirectory; }
    std::string &currentDirectory()
    { return _currentDirectory; }

public:
    virtual std::vector<std::string> commandLineArguments() const
    { return _commandLineArguments; }
    std::vector<std::string> &commandLineArguments()
    { return _commandLineArguments; }

    virtual std::unordered_map<std::string, std::string> environmentVariables() const
    { return _environmentVariables; }
    std::unordered_map<std::string, std::string> &environmentVariables()
    { return _environmentVariables; }

    virtual ext::optional<std::string> environmentVariable(std::string const &variable) const;

public:
    virtual int32_t userID() const
    { return _userID; }
    int32_t &userID()
    { return _userID; }

    virtual int32_t groupID() const
    { return _groupID; }
    int32_t &groupID()
    { return _groupID; }

    virtual std::string userName() const
    { return _userName; }
    std::string &userName()
    { return _userName; }

    virtual std::string groupName() const
    { return _groupName; }
    std::string &groupName()
    { return _groupName; }
};

}

#endif  // !__libutil_MemoryProcessContext_h
