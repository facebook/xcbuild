/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef __pbxbuild_Tool_Context_h
#define __pbxbuild_Tool_Context_h

#include <pbxbuild/Base.h>
#include <pbxbuild/Tool/Invocation.h>
#include <pbxbuild/Tool/HeadermapInfo.h>
#include <pbxbuild/Tool/CompilationInfo.h>
#include <pbxbuild/Tool/SwiftModuleInfo.h>
#include <pbxbuild/Tool/PrecompiledHeaderInfo.h>
#include <pbxbuild/Tool/SearchPaths.h>
#include <xcsdk/SDK/Target.h>
#include <xcsdk/SDK/Toolchain.h>

namespace pbxbuild {
namespace Tool {

class Context {
private:
    xcsdk::SDK::Target::shared_ptr      _sdk;
    xcsdk::SDK::Toolchain::vector       _toolchains;
    std::vector<std::string>            _executablePaths;
    std::string                         _workingDirectory;

private:
    SearchPaths                         _searchPaths;

private:
    HeadermapInfo                       _headermapInfo;
    CompilationInfo                     _compilationInfo;
    std::vector<SwiftModuleInfo>        _swiftModuleInfo;
    std::vector<std::string>            _additionalInfoPlistContents;

private:
    std::vector<Tool::Invocation> _invocations;
    std::map<std::pair<std::string, std::string>, std::vector<Tool::Invocation>> _variantArchitectureInvocations;

public:
    Context(
        xcsdk::SDK::Target::shared_ptr const &sdk,
        xcsdk::SDK::Toolchain::vector const &toolchains,
        std::vector<std::string> const &executablePaths,
        std::string const &workingDirectory,
        SearchPaths const &searchPaths);
    ~Context();

public:
    xcsdk::SDK::Target::shared_ptr const &sdk() const
    { return _sdk; }
    xcsdk::SDK::Toolchain::vector const &toolchains() const
    { return _toolchains; }
    std::vector<std::string> const &executablePaths() const
    { return _executablePaths; }
    std::string const &workingDirectory() const
    { return _workingDirectory; }

public:
    SearchPaths const &searchPaths() const
    { return _searchPaths; }

public:
    HeadermapInfo const &headermapInfo() const
    { return _headermapInfo; }
    CompilationInfo const &compilationInfo() const
    { return _compilationInfo; }
    std::vector<SwiftModuleInfo> const &swiftModuleInfo() const
    { return _swiftModuleInfo; }
    std::vector<std::string> const &additionalInfoPlistContents() const
    { return _additionalInfoPlistContents; }

public:
    HeadermapInfo &headermapInfo()
    { return _headermapInfo; }
    CompilationInfo &compilationInfo()
    { return _compilationInfo; }
    std::vector<SwiftModuleInfo> &swiftModuleInfo()
    { return _swiftModuleInfo; }
    std::vector<std::string> &additionalInfoPlistContents()
    { return _additionalInfoPlistContents; }

public:
    std::vector<Tool::Invocation> const &invocations() const
    { return _invocations; }
    std::map<std::pair<std::string, std::string>, std::vector<Tool::Invocation>> const &variantArchitectureInvocations() const
    { return _variantArchitectureInvocations; }

public:
    std::vector<Tool::Invocation> &invocations()
    { return _invocations; }
    std::map<std::pair<std::string, std::string>, std::vector<Tool::Invocation>> &variantArchitectureInvocations()
    { return _variantArchitectureInvocations; }
};

}
}

#endif // !__pbxbuild_Tool_Context_h
