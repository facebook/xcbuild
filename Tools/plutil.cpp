/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#include <plist/plist.h>
#include <libutil/Options.h>
#include <libutil/FSUtil.h>

#include <fstream>

class Options {
public:
    class Adjustment {
    public:
        enum class Type {
            Extract,
            Replace,
            Insert,
            Remove,
        };

    private:
        Type                                 _type;
        std::string                          _path;
        std::unique_ptr<plist::Object const> _value;

    public:
        Adjustment(Type type, std::string const &path, std::unique_ptr<plist::Object const> value);

    public:
        Type type() const
        { return _type; }
        std::string const &path() const
        { return _path; }
        plist::Object const *value() const
        { return _value.get(); }
    };

private:
    bool                     _help;
    bool                     _print;
    bool                     _lint;

private:
    std::shared_ptr<plist::Format::Type> _convert;

public:
    std::vector<Adjustment>  _adjustments;

private:
    std::vector<std::string> _inputs;
    std::string              _output;
    std::string              _extension;
    bool                     _separator;

private:
    bool                     _silent;
    bool                     _humanReadable;

public:
    Options();
    ~Options();

public:
    bool help() const
    { return _help; }
    bool print() const
    { return _print; }
    bool lint() const
    { return _lint; }

public:
    std::shared_ptr<plist::Format::Type> const &convert() const
    { return _convert; }

public:
    std::vector<Adjustment> const &adjustments() const
    { return _adjustments; }

public:
    std::vector<std::string> const &inputs() const
    { return _inputs; }
    std::string const &output() const
    { return _output; }
    std::string const &extension() const
    { return _extension; }

public:
    bool silent() const
    { return _silent; }
    bool humanReadable() const
    { return _humanReadable; }

private:
    friend class libutil::Options;
    std::pair<bool, std::string>
    parseArgument(std::vector<std::string> const &args, std::vector<std::string>::const_iterator *it);
};

Options::
Options() :
    _help         (false),
    _print        (false),
    _lint         (false),
    _separator    (false),
    _silent       (false),
    _humanReadable(false)
{
}

Options::
~Options()
{
}

Options::Adjustment::
Adjustment(Type type, std::string const &path, std::unique_ptr<plist::Object const> value) :
    _type (type),
    _path (path),
    _value(std::move(value))
{
}

static std::pair<bool, std::string>
NextFormatType(plist::Format::Type *type, std::vector<std::string> const &args, std::vector<std::string>::const_iterator *it)
{
    std::string format;
    std::pair<bool, std::string> result = libutil::Options::NextString(&format, args, it);
    if (result.first) {
        if (format == "xml1") {
            *type = plist::Format::Type::XML;
        } else if (format == "binary1") {
            *type = plist::Format::Type::Binary;
        } else if (format == "openstep1" || format == "ascii1") {
            *type = plist::Format::Type::ASCII;
        } else if (format == "json") {
            // TODO(grp): Support JSON output.
            return std::make_pair(false, "JSON not yet implemented");
        } else {
            return std::make_pair(false, "unknown format " + format);
        }
    }
    return result;
}

static std::pair<bool, std::string>
SanitizeXMLFormat(std::string *value)
{
    std::string::size_type body = 0;

    std::string xml = "<?xml";
    if (value->compare(body, xml.size(), xml) == 0) {
        /* Already has XML header. */
        body = value->find('>', body);
        if (body == std::string::npos || (++body) > value->size()) {
            return std::make_pair(false, "invalid XML header");
        }
    } else {
        /* Add XML header. */
        std::string header = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        value->insert(body, header);
        body += header.size();
    }

    std::string doctype = "<!DOCTYPE";
    if (value->compare(body, doctype.size(), doctype) == 0) {
        /* Already has a doctype. */
        body = value->find('>', body);
        if (body == std::string::npos || (++body) > value->size()) {
            return std::make_pair(false, "invalid doctype header");
        }
    } else {
        /* Add plist doctype. */
        std::string header = "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">";
        value->insert(body, header);
        body += header.size();
    }

    std::string plist = "<plist";
    if (value->compare(body, plist.size(), plist) == 0) {
        /* Already has <plist> element. */
        body = value->find('>', body);
        if (body == std::string::npos || (++body) > value->size()) {
            return std::make_pair(false, "invalid plist header");
        }
    } else {
        /* Wrap with <plist></plist> element. */
        std::string header = "<plist version=\"1.0\">";
        value->insert(body, header);
        body += header.size();

        value->insert(value->size(), "</plist>");
    }

    return std::make_pair(true, std::string());
}

static std::pair<bool, std::string>
NextAdjustment(Options::Adjustment *adjustment, Options::Adjustment::Type type, std::vector<std::string> const &args, std::vector<std::string>::const_iterator *it)
{
    std::pair<bool, std::string> result;

    std::string path;
    result = libutil::Options::NextString(&path, args, it);
    if (!result.first) {
        return result;
    }

    std::string arg;
    result = libutil::Options::NextString(&arg, args, it);
    if (!result.first) {
        return result;
    }

    std::string value;
    result = libutil::Options::NextString(&value, args, it);
    if (!result.first) {
        return result;
    }

    std::unique_ptr<plist::Object const> object = nullptr;

    if (arg == "-bool") {
        bool boolean = (value == "YES" || value == "true");
        object = plist::Boolean::New(boolean);
    } else if (arg == "-integer") {
        char *end = NULL;
        long long integer = std::strtoll(value.c_str(), &end, 0);

        if (end != value.c_str()) {
            object = plist::Integer::New(integer);
        } else {
            return std::make_pair(false, "invalid integer argument");
        }
    } else if (arg == "-float") {
        char *end = NULL;
        double real = std::strtod(value.c_str(), &end);

        if (end != value.c_str()) {
            object = plist::Real::New(real);
        } else {
            return std::make_pair(false, "invalid float argument");
        }
    } else if (arg == "-string") {
        object = plist::String::New(value);
    } else if (arg == "-date") {
        object = plist::Date::New(value);
    } else if (arg == "-data") {
        object = plist::Data::New(value);
    } else if (arg == "-xml") {
        std::pair<bool, std::string> result = SanitizeXMLFormat(&value);
        if (!result.first) {
            return result;
        }

        std::vector<uint8_t> contents = std::vector<uint8_t>(value.begin(), value.end());
        plist::Format::XML format = plist::Format::XML::Create(plist::Format::Encoding::UTF8);

        auto deserialize = plist::Format::XML::Deserialize(contents, format);
        if (deserialize.first == nullptr) {
            return std::make_pair(false, deserialize.second);
        }

        object = std::move(deserialize.first);
    } else if (arg == "-json") {
        return std::make_pair(false, "JSON not yet implemented");
    } else {
        return std::make_pair(false, "unknown type option " + arg);
    }

    *adjustment = Options::Adjustment(type, path, std::move(object));

    return result;
}

std::pair<bool, std::string> Options::
parseArgument(std::vector<std::string> const &args, std::vector<std::string>::const_iterator *it)
{
    std::string const &arg = **it;

    if (_separator) {
        _inputs.push_back(arg);
        return std::make_pair(true, std::string());
    }

    if (arg == "-help") {
        return libutil::Options::MarkBool(&_help, arg, it);
    } else if (arg == "-lint") {
        return libutil::Options::MarkBool(&_lint, arg, it);
    } else if (arg == "-p") {
        return libutil::Options::MarkBool(&_print, arg, it);
    } else if (arg == "-convert") {
        plist::Format::Type type;
        std::pair<bool, std::string> result = NextFormatType(&type, args, it);
        if (result.first) {
            _convert = std::make_shared<plist::Format::Type>(type);
        }
        return result;
    } else if (arg == "-insert") {
        Options::Adjustment adjustment = Options::Adjustment(Options::Adjustment::Type::Insert, std::string(), nullptr);
        std::pair<bool, std::string> result = NextAdjustment(&adjustment, Options::Adjustment::Type::Insert, args, it);
        if (result.first) {
            _adjustments.emplace_back(std::move(adjustment));
        }
        return result;
    } else if (arg == "-replace") {
        Options::Adjustment adjustment = Options::Adjustment(Options::Adjustment::Type::Replace, std::string(), nullptr);
        std::pair<bool, std::string> result = NextAdjustment(&adjustment, Options::Adjustment::Type::Replace, args, it);
        if (result.first) {
            _adjustments.emplace_back(std::move(adjustment));
        }
        return result;
    } else if (arg == "-remove") {
        std::string remove;
        std::pair<bool, std::string> result = libutil::Options::NextString(&remove, args, it);
        if (result.first) {
            Options::Adjustment adjustment = Options::Adjustment(Options::Adjustment::Type::Remove, remove, nullptr);
            _adjustments.emplace_back(std::move(adjustment));
        }
        return result;
    } else if (arg == "-extract") {
        std::string path;
        std::pair<bool, std::string> result = libutil::Options::NextString(&path, args, it);
        if (!result.first) {
            return result;
        }

        Options::Adjustment adjustment = Options::Adjustment(Options::Adjustment::Type::Extract, path, nullptr);
        _adjustments.emplace_back(std::move(adjustment));

        plist::Format::Type type;
        result = NextFormatType(&type, args, it);
        if (!result.first) {
            return result;
        }

        _convert = std::make_shared<plist::Format::Type>(type);
        return result;
    } else if (arg == "-e") {
        return libutil::Options::NextString(&_extension, args, it);
    } else if (arg == "-o") {
        return libutil::Options::NextString(&_output, args, it);
    } else if (arg == "-s") {
        return libutil::Options::MarkBool(&_silent, arg, it);
    } else if (arg == "-r") {
        return libutil::Options::MarkBool(&_humanReadable, arg, it);
    } else if (arg == "--") {
        return libutil::Options::MarkBool(&_separator, arg, it);
    } else if (!arg.empty() && arg[0] != '-') {
        _inputs.push_back(arg);
        return std::make_pair(true, std::string());
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

    fprintf(stderr, "usage: plutil -<command> [options] <files>\n");

#define INDENT "  "
    fprintf(stderr, "\ncommands:\n");
    fprintf(stderr, INDENT "-lint (default)\n");
    fprintf(stderr, INDENT "-help (this message)\n");
    fprintf(stderr, INDENT "-p\n");
    fprintf(stderr, INDENT "-convert <format>\n");
    fprintf(stderr, INDENT "-insert <key> <value>\n");
    fprintf(stderr, INDENT "-replace <key> <value>\n");
    fprintf(stderr, INDENT "-remove <key>\n");
    fprintf(stderr, INDENT "-extract <key> <format>\n");

    fprintf(stderr, "\nvalues:\n");
    fprintf(stderr, INDENT "-bool <YES|NO>\n");
    fprintf(stderr, INDENT "-integer <number>\n");
    fprintf(stderr, INDENT "-float <number>\n");
    fprintf(stderr, INDENT "-string <utf8>\n");
    fprintf(stderr, INDENT "-data <base64>\n");
    fprintf(stderr, INDENT "-date <iso8601>\n");
    fprintf(stderr, INDENT "-xml <plist>\n");
    fprintf(stderr, INDENT "-json <json> (not yet implemented)\n");

    fprintf(stderr, "\nformats:\n");
    fprintf(stderr, INDENT "xml1\n");
    fprintf(stderr, INDENT "binary1\n");
    fprintf(stderr, INDENT "openstep1\n");
    fprintf(stderr, INDENT "json (not yet implemented)\n");
#undef INDENT

    return (error.empty() ? 0 : -1);
}

static std::pair<bool, std::vector<uint8_t>>
Read(std::string const &path = "-")
{
    std::vector<uint8_t> contents;

    if (path == "-") {
        /* - means read from stdin. */
        contents = std::vector<uint8_t>(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
    } else {
        /* Read from file. */
        std::ifstream file = std::ifstream(path, std::ios::binary);
        if (file.fail()) {
            return std::make_pair(false, std::vector<uint8_t>());
        }

        contents = std::vector<uint8_t>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }

    return std::make_pair(true, std::move(contents));
}

static bool
Write(std::vector<uint8_t> const &contents, std::string const &path = "-")
{
    if (path == "-") {
        /* - means write to stdout. */
        std::copy(contents.begin(), contents.end(), std::ostream_iterator<char>(std::cout));
    } else {
        /* Read from file. */
        std::ofstream file = std::ofstream(path, std::ios::binary);
        if (file.fail()) {
            return false;
        }

        std::copy(contents.begin(), contents.end(), std::ostream_iterator<char>(file));
    }

    return true;
}

static bool
Lint(Options const &options, std::string const &file)
{
    if (!options.silent()) {
        /* Already linted by virtue of getting this far. */
        printf("%s: OK\n", file.c_str());
    }

    return true;
}

static bool
Print(Options const &options, std::unique_ptr<plist::Object> object, plist::Format::Any const &format)
{
    /* Convert to ASCII. */
    plist::Format::ASCII out = plist::Format::ASCII::Create(false, plist::Format::Encoding::UTF8);
    auto serialize = plist::Format::ASCII::Serialize(object.get(), out);
    if (serialize.first == nullptr) {
        fprintf(stderr, "error: %s\n", serialize.second.c_str());
        return false;
    }

    /* Print. */
    if (!Write(*serialize.first)) {
        fprintf(stderr, "error: unable to write\n");
        return false;
    }

    return true;
}

static std::string
OutputPath(Options const &options, std::string const &file)
{
    if (!options.output().empty()) {
        /* A specified output path. */
        return options.output();
    }

    if (file != "-" && !options.extension().empty()) {
        /* Replace the file extension with the provided one. */
        return libutil::FSUtil::GetDirectoryName(file) + "/" + libutil::FSUtil::GetBaseNameWithoutExtension(file) + "." + options.extension();
    }

    /* Default to overwriting the input. */
    return file;
}

static void
PerformAdjustment(plist::Object *object, plist::Object **rootObject, std::string const &key, Options::Adjustment const &adjustment)
{
    if (plist::Dictionary *dict = plist::CastTo<plist::Dictionary>(object)) {
        switch (adjustment.type()) {
            case Options::Adjustment::Type::Insert:
                /* Only insert if doesn't already exist. */
                if (dict->value(key) == nullptr) {
                    dict->set(key, adjustment.value()->copy());
                }
                break;
            case Options::Adjustment::Type::Replace:
                /* Insert or replace as needed. */
                dict->set(key, adjustment.value()->copy());
                break;
            case Options::Adjustment::Type::Remove:
                dict->remove(key);
                break;
            case Options::Adjustment::Type::Extract:
                *rootObject = dict->value(key);
                break;
        }
    } else if (plist::Array *array = plist::CastTo<plist::Array>(object)) {
        uint64_t index = std::stoull(key.c_str(), NULL, 0);

        switch (adjustment.type()) {
            case Options::Adjustment::Type::Insert: {
                /* Insert within the array, otherwise append. */
                if (index < array->count()) {
                    array->insert(index, adjustment.value()->copy());
                } else {
                    array->append(adjustment.value()->copy());
                }
                break;
            }
            case Options::Adjustment::Type::Replace: {
                /* Replace within the array, otherwise append. */
                if (index < array->count()) {
                    array->set(index, adjustment.value()->copy());
                } else {
                    array->append(adjustment.value()->copy());
                }
                break;
            }
            case Options::Adjustment::Type::Remove: {
                if (index < array->count()) {
                    array->remove(index);
                }
                break;
            }
            case Options::Adjustment::Type::Extract: {
                *rootObject = array->value(index);
                break;
            }
        }
    }
}

static bool
Modify(Options const &options, std::string const &file, std::unique_ptr<plist::Object> object, plist::Format::Any const &format)
{
    plist::Object *writeObject = object.get();

    /* Apply requested adjustments. */
    for (Options::Adjustment const &adjustment : options.adjustments()) {
        plist::Object *currentObject = writeObject;

        std::string path = adjustment.path();
        std::string::size_type start = 0;
        std::string::size_type end = 0;

        do {
            end = path.find('.', end);
            std::string key = (end != std::string::npos ? path.substr(start, end - start) : path.substr(start));

            if (end != std::string::npos) {
                /* Intermediate key path: continue iterating. */
                if (plist::Dictionary *dict = plist::CastTo<plist::Dictionary>(currentObject)) {
                    currentObject = dict->value(key);
                } else if (plist::Array *array = plist::CastTo<plist::Array>(currentObject)) {
                    uint64_t index = std::stoull(key.c_str(), NULL, 0);
                    currentObject = array->value(index);
                }
            } else {
                /* Final key path: perform the action. */
                PerformAdjustment(currentObject, &writeObject, key, adjustment);
            }

            if (currentObject == nullptr) {
                fprintf(stderr, "error: invalid key path\n");
                return false;
            }

            start = end + 1;
        } while (end != std::string::npos);
    }

    /* Find output format. */
    plist::Format::Any out = format;
    if (options.convert() != nullptr) {
        switch (*options.convert()) {
            case plist::Format::Type::Binary: {
                plist::Format::Binary binary = plist::Format::Binary::Create();
                out = plist::Format::Any::Create<plist::Format::Binary>(binary);
                break;
            }
            case plist::Format::Type::XML: {
                plist::Format::XML xml = plist::Format::XML::Create(plist::Format::Encoding::UTF8);
                out = plist::Format::Any::Create<plist::Format::XML>(xml);
                break;
            }
            case plist::Format::Type::ASCII: {
                plist::Format::ASCII ascii = plist::Format::ASCII::Create(false, plist::Format::Encoding::UTF8);
                out = plist::Format::Any::Create<plist::Format::ASCII>(ascii);
                break;
            }
            default: abort();
        }
    }

    /* Convert to desired format. */
    auto serialize = plist::Format::Any::Serialize(writeObject, out);
    if (serialize.first == nullptr) {
        fprintf(stderr, "error: %s\n", serialize.second.c_str());
        return false;
    }

    /* Write to output. */
    std::string output = OutputPath(options, file);
    if (!Write(*serialize.first, output)) {
        fprintf(stderr, "error: unable to write\n");
        return false;
    }

    return true;
}

int
main(int argc, char **argv)
{
    std::vector<std::string> args = std::vector<std::string>(argv + 1, argv + argc);

    Options options;
    std::pair<bool, std::string> result = libutil::Options::Parse<Options>(&options, args);
    if (!result.first) {
        return Help(result.second);
    }

    /* Detect conflicting mode options. */
    bool modify = (options.convert() != nullptr || options.adjustments().size() > 0);
    if ((modify && options.lint()) ||
        (modify && options.print()) ||
        (modify && options.help()) ||
        (options.lint() && options.print()) ||
        (options.print() && options.help()) ||
        (options.lint() && options.help())) {
        return Help("conflicting options specified");
    }

    /* Perform actions. */
    if (options.help()) {
        return Help();
    } else {
        bool success = true;

        if (options.inputs().empty()) {
            return Help("no input files");
        }

        /* Actions applied to each input file separately. */
        for (std::string const &file : options.inputs()) {
            std::pair<bool, std::vector<uint8_t>> result = Read(file);
            if (!result.first) {
                fprintf(stderr, "error: unable to read %s\n", file.c_str());
                success = false;
                continue;
            }

            auto format = plist::Format::Any::Identify(result.second);
            if (format == nullptr) {
                fprintf(stderr, "error: input %s not a plist\n", file.c_str());
                success = false;
                continue;
            }

            auto deserialize = plist::Format::Any::Deserialize(result.second, *format);
            if (!deserialize.first) {
                fprintf(stderr, "error: %s\n", deserialize.second.c_str());
                success = false;
                continue;
            }

            /* Perform the sepcific action. */
            if (modify) {
                success = success && Modify(options, file, std::move(deserialize.first), *format);
            } else if (options.print()) {
                success = success && Print(options, std::move(deserialize.first), *format);
            } else if (options.lint() || true) {
                success = success && Lint(options, file);
            }
        }

        return (success ? 1 : 0);
    }
}
