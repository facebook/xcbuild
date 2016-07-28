/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#include <acdriver/CompileOutput.h>
#include <acdriver/Options.h>
#include <acdriver/Result.h>
#include <xcassets/Asset/Asset.h>
#include <libutil/Filesystem.h>
#include <plist/Format/Format.h>
#include <plist/Format/XML.h>

using acdriver::CompileOutput;
using acdriver::Options;
using acdriver::Result;
using libutil::Filesystem;

CompileOutput::
CompileOutput(std::string const &root, Format format) :
    _root          (root),
    _format        (format),
    _additionalInfo(plist::Dictionary::New())
{
}

bool CompileOutput::
write(Filesystem *filesystem, ext::optional<std::string> const &partialInfoPlist, ext::optional<std::string> const &dependencyInfo, Result *result) const
{
    bool success = true;

    /*
     * Write out compiled archive.
     */
    if (_car) {
        // TODO: only write if non-empty. but did mmap already create the file?
        _car->write();
    }

    /*
     * Copy files into output.
     */
    for (std::pair<std::string, std::string> const &copy : _copies) {
        std::vector<uint8_t> contents;

        if (!filesystem->read(&contents, copy.first)) {
            result->normal(Result::Severity::Error, "unable to read input: " + copy.first);
            success = false;
            continue;
        }

        if (!filesystem->write(contents, copy.second)) {
            result->normal(Result::Severity::Error, "unable to write output: " + copy.second);
            success = false;
            continue;
        }
    }

    /*
     * Write out partial info plist, if requested.
     */
    if (partialInfoPlist) {
        auto format = plist::Format::XML::Create(plist::Format::Encoding::UTF8);
        auto serialize = plist::Format::XML::Serialize(_additionalInfo.get(), format);
        if (serialize.first == nullptr) {
            result->normal(Result::Severity::Error, "unable to serialize partial info plist");
            success = false;
        } else {
            if (!filesystem->write(*serialize.first, *partialInfoPlist)) {
                result->normal(Result::Severity::Error, "unable to write partial info plist");
                success = false;
            }
        }
    }

    /*
     * Write out dependency info, if requested.
     */
    if (dependencyInfo) {
        auto contents = _dependencyInfo.serialize();
        if (contents.size() > 0 && !filesystem->write(contents, *dependencyInfo)) {
            result->normal(Result::Severity::Error, "unable to write dependency info");
            success = false;
        }
    }

    return success;
}

std::string CompileOutput::
AssetReference(std::shared_ptr<xcassets::Asset::Asset> const &asset)
{
    // TODO: include [] for each key
    return asset->path();
}

