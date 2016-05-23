/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#include <libutil/Options.h>
#include <libutil/Escape.h>
#include <libutil/FSUtil.h>

#include <dependency/DependencyInfo.h>
#include <dependency/BinaryDependencyInfo.h>
#include <dependency/DirectoryDependencyInfo.h>
#include <dependency/MakefileDependencyInfo.h>

#include <sstream>
#include <fstream>
#include <iterator>

#include <cassert>

using libutil::FSUtil;
using libutil::Escape;

class Options {
private:
    bool        _help;
    bool        _version;

private:
    std::vector<std::pair<dependency::DependencyInfoFormat, std::string>> _inputs;
    std::string _output;
    std::string _name;

public:
    Options();
    ~Options();

public:
    bool help() const
    { return _help; }
    bool version() const
    { return _version; }

public:
    std::vector<std::pair<dependency::DependencyInfoFormat, std::string>> const &inputs() const
    { return _inputs; }
    std::string const &output() const
    { return _output; }
    std::string const &name() const
    { return _name; }

private:
    friend class libutil::Options;
    std::pair<bool, std::string>
    parseArgument(std::vector<std::string> const &args, std::vector<std::string>::const_iterator *it);
};

Options::
Options() :
    _help                  (false),
    _version               (false)
{
}

Options::
~Options()
{
}

std::pair<bool, std::string> Options::
parseArgument(std::vector<std::string> const &args, std::vector<std::string>::const_iterator *it)
{
    std::string const &arg = **it;

    if (arg == "-h" || arg == "--help") {
        return libutil::Options::MarkBool(&_help, arg, it);
    } else if (arg == "-v" || arg == "--version") {
        return libutil::Options::MarkBool(&_version, arg, it);
    } else if (arg == "-o" || arg == "--output") {
        return libutil::Options::NextString(&_output, args, it);
    } else if (arg == "-n" || arg == "--name") {
        return libutil::Options::NextString(&_name, args, it);
    } else if (!arg.empty() && arg[0] != '-') {
        std::string::size_type offset = arg.find(':');
        if (offset != std::string::npos && offset != 0 && offset != arg.size() - 1) {
            std::string name = arg.substr(0, offset);
            std::string path = arg.substr(offset + 1);

            dependency::DependencyInfoFormat format;
            if (!dependency::DependencyInfoFormats::Parse(name, &format)) {
                return std::make_pair(false, "unknown format "+ name);
            }

            _inputs.push_back({ format, path });
            return std::make_pair(true, std::string());
        } else {
            return std::make_pair(false, "unknown input "+ arg + " (use format:/path/to/input)");
        }
    } else {
        return std::make_pair(false, "unknown argument " + arg);
    }
}

static int
Help(std::string const &error = std::string())
{
    if (!error.empty()) {
        fprintf(stderr, "error: %s\n", error.c_str());
        fprintf(stderr, "\n");
    }

    fprintf(stderr, "Usage: dependency-info-tool [options]\n\n");
    fprintf(stderr, "Converts dependency info to Ninja format.\n\n");

#define INDENT "  "
    fprintf(stderr, "Information:\n");
    fprintf(stderr, INDENT "-h, --help\n");
    fprintf(stderr, INDENT "-v, --version\n");
    fprintf(stderr, "\n");

    fprintf(stderr, "Conversion Options:\n");
    fprintf(stderr, INDENT "-i, --input\n");
    fprintf(stderr, INDENT "-o, --output\n");
    fprintf(stderr, INDENT "-f, --format\n");
    fprintf(stderr, INDENT "-n, --name\n");
    fprintf(stderr, "\n");
#undef INDENT

    return (error.empty() ? 0 : -1);
}

static int
Version()
{
    printf("ninja-dependency-info version 1 (xcbuild)\n");
    return 0;
}

static bool
LoadDependencyInfo(std::string const &path, dependency::DependencyInfoFormat format, std::vector<dependency::DependencyInfo> *dependencyInfo)
{
    if (format == dependency::DependencyInfoFormat::Binary) {
        std::ifstream input(path, std::ios::binary);
        if (input.fail()) {
            fprintf(stderr, "error: failed to open %s\n", path.c_str());
            return false;
        }

        std::vector<uint8_t> contents = std::vector<uint8_t>(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());

        auto binaryInfo = dependency::BinaryDependencyInfo::Deserialize(contents);
        if (!binaryInfo) {
            fprintf(stderr, "error: invalid binary dependency info\n");
            return false;
        }

        dependencyInfo->push_back(binaryInfo->dependencyInfo());
        return true;
    } else if (format == dependency::DependencyInfoFormat::Directory) {
        auto directoryInfo = dependency::DirectoryDependencyInfo::Deserialize(path);
        if (!directoryInfo) {
            fprintf(stderr, "error: invalid directory\n");
            return false;
        }

        dependencyInfo->push_back(directoryInfo->dependencyInfo());
        return true;
    } else if (format == dependency::DependencyInfoFormat::Makefile) {
        std::ifstream input(path, std::ios::binary);
        if (input.fail()) {
            fprintf(stderr, "error: failed to open %s\n", path.c_str());
            return false;
        }

        std::string contents = std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());

        auto makefileInfo = dependency::MakefileDependencyInfo::Deserialize(contents);
        if (!makefileInfo) {
            fprintf(stderr, "error: invalid makefile dependency info\n");
            return false;
        }

        dependencyInfo->insert(dependencyInfo->end(), makefileInfo->dependencyInfo().begin(), makefileInfo->dependencyInfo().end());
        return true;
    } else {
        assert(false);
        return false;
    }
}

static std::string
SerializeMakefileDependencyInfo(std::string const &output, std::vector<std::string> const &inputs)
{
    dependency::DependencyInfo dependencyInfo;
    dependencyInfo.outputs() = { output };

    /* Normalize path as Ninja requires matching paths. */
    std::string currentDirectory = FSUtil::GetCurrentDirectory();
    for (std::string const &input : inputs) {
        std::string path = FSUtil::ResolveRelativePath(input, currentDirectory);
        dependencyInfo.inputs().push_back(path);
    }

    /* Serialize dependency info. */
    dependency::MakefileDependencyInfo makefileInfo;
    makefileInfo.dependencyInfo() = { dependencyInfo };
    return makefileInfo.serialize();
}

int
main(int argc, char **argv)
{
    std::vector<std::string> args = std::vector<std::string>(argv + 1, argv + argc);

    /*
     * Parse out the options, or print help & exit.
     */
    Options options;
    std::pair<bool, std::string> result = libutil::Options::Parse<Options>(&options, args);
    if (!result.first) {
        return Help(result.second);
    }

    /*
     * Handle the basic options.
     */
    if (options.help()) {
        return Help();
    } else if (options.version()) {
        return Version();
    }

    /*
     * Diagnose missing options.
     */
    if (options.inputs().empty() || options.output().empty() || options.name().empty()) {
        return Help("missing option(s)");
    }

    std::vector<std::string> inputs;
    for (std::pair<dependency::DependencyInfoFormat, std::string> const &input : options.inputs()) {
        /*
         * Load the dependency info.
         */
        std::vector<dependency::DependencyInfo> info;
        if (!LoadDependencyInfo(input.second, input.first, &info)) {
            return -1;
        }

        for (dependency::DependencyInfo const &dependencyInfo : info) {
            inputs.insert(inputs.end(), dependencyInfo.inputs().begin(), dependencyInfo.inputs().end());
        }
    }

    /*
     * Serialize the output.
     */
    std::string contents = SerializeMakefileDependencyInfo(options.name(), inputs);

    /*
     * Write out the output.
     */
    std::ofstream output(options.output(), std::ios::binary);
    if (output.fail()) {
        return false;
    }

    std::copy(contents.begin(), contents.end(), std::ostream_iterator<char>(output));

    return 0;
}


