/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#include <xcassets/Asset/StickerSequence.h>
#include <plist/Keys/Unpack.h>
#include <plist/Array.h>
#include <plist/Real.h>
#include <plist/String.h>
#include <libutil/Filesystem.h>

#include <cstdlib>

using xcassets::Asset::StickerSequence;
using DurationType = StickerSequence::DurationType;
using DurationTypes = StickerSequence::DurationTypes;
using libutil::Filesystem;

bool StickerSequence::
load(Filesystem const *filesystem)
{
    if (!Asset::load(filesystem)) {
        return false;
    }

    if (this->hasChildren(filesystem)) {
        fprintf(stderr, "warning: unexpected child assets\n");
    }

    return true;
}

bool StickerSequence::Frame::
parse(plist::Dictionary const *dict)
{
    std::unordered_set<std::string> seen;
    auto unpack = plist::Keys::Unpack("StickerSequenceFrame", dict, &seen);

    auto F = unpack.cast <plist::String> ("filename");

    if (!unpack.complete(true)) {
        fprintf(stderr, "%s", unpack.errorText().c_str());
    }

    if (F != nullptr) {
        _fileName = F->value();
    }

    return true;
}

bool StickerSequence::
parse(plist::Dictionary const *dict, std::unordered_set<std::string> *seen, bool check)
{
    if (!Asset::parse(dict, seen, false)) {
        return false;
    }

    /* Contents is required. */
    if (dict == nullptr) {
        return false;
    }

    auto unpack = plist::Keys::Unpack("StickerSequence", dict, seen);

    auto P  = unpack.cast <plist::Dictionary> ("properties");
    auto Fs = unpack.cast <plist::Array> ("frames");

    if (!unpack.complete(check)) {
        fprintf(stderr, "%s", unpack.errorText().c_str());
    }

    if (P != nullptr) {
        std::unordered_set<std::string> seen;
        auto unpack = plist::Keys::Unpack("Properties", P, &seen);

        auto AL = unpack.cast <plist::String> ("accessibility-label");
        auto D  = unpack.cast <plist::Real> ("duration");
        auto DT = unpack.cast <plist::String> ("duration-type");
        auto R  = unpack.cast <plist::Real> ("repetitions");

        if (!unpack.complete(true)) {
            fprintf(stderr, "%s", unpack.errorText().c_str());
        }

        if (AL != nullptr) {
            _accessibilityLabel = AL->value();
        }

        if (D != nullptr) {
            _duration = D->value();
        }

        if (DT != nullptr) {
            _durationType = DurationTypes::Parse(DT->value());
        }

        if (R != nullptr) {
            _repetitions = R->value();
        }
    }

    if (Fs != nullptr) {
        _frames = std::vector<Frame>();

        for (size_t n = 0; n < Fs->count(); ++n) {
            if (auto dict = Fs->value<plist::Dictionary>(n)) {
                Frame frame;
                if (frame.parse(dict)) {
                    _frames->push_back(frame);
                }
            }
        }
    }

    return true;
}

ext::optional<DurationType> DurationTypes::
Parse(std::string const &value)
{
    if (value == "fixed") {
        return DurationType::Fixed;
    } else if (value == "fps") {
        return DurationType::FPS;
    } else {
        fprintf(stderr, "warning: unknown platform %s\n", value.c_str());
        return ext::nullopt;
    }
}

std::string DurationTypes::
String(DurationType platform)
{
    switch (platform) {
        case DurationType::Fixed:
            return "fixed";
        case DurationType::FPS:
            return "fps";
    }

    abort();
}
