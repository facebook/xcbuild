/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#include <acdriver/CompileAction.h>
#include <acdriver/Compile/Output.h>
#include <acdriver/Compile/AppIconSet.h>
#include <acdriver/Compile/BrandAssets.h>
#include <acdriver/Compile/ComplicationSet.h>
#include <acdriver/Compile/DataSet.h>
#include <acdriver/Compile/GCDashboardImage.h>
#include <acdriver/Compile/GCLeaderboard.h>
#include <acdriver/Compile/GCLeaderboardSet.h>
#include <acdriver/Compile/IconSet.h>
#include <acdriver/Compile/ImageSet.h>
#include <acdriver/Compile/ImageStack.h>
#include <acdriver/Compile/ImageStackLayer.h>
#include <acdriver/Compile/LaunchImage.h>
#include <acdriver/Compile/SpriteAtlas.h>
#include <acdriver/Version.h>
#include <acdriver/Options.h>
#include <acdriver/Output.h>
#include <acdriver/Result.h>
#include <xcassets/Asset/Catalog.h>
#include <xcassets/Asset/Group.h>
#include <bom/bom.h>
#include <car/Reader.h>
#include <car/Writer.h>
#include <bom/bom_format.h>
#include <dependency/BinaryDependencyInfo.h>
#include <plist/Array.h>
#include <plist/Boolean.h>
#include <plist/Dictionary.h>
#include <plist/String.h>
#include <plist/Format/XML.h>
#include <libutil/Filesystem.h>
#include <libutil/FSUtil.h>

using acdriver::CompileAction;
namespace Compile = acdriver::Compile;
using acdriver::Version;
using acdriver::Options;
using acdriver::Output;
using acdriver::Result;
using libutil::Filesystem;
using libutil::FSUtil;

CompileAction::
CompileAction()
{
}

CompileAction::
~CompileAction()
{
}

static bool
CompileAsset(
    std::shared_ptr<xcassets::Asset::Asset> const &asset,
    std::shared_ptr<xcassets::Asset::Asset> const &parent,
    Filesystem *filesystem,
    Options const &options,
    Compile::Output *compileOutput,
    Result *result);

template<typename T>
static bool
CompileChildren(
    std::vector<T> const &assets,
    std::shared_ptr<xcassets::Asset::Asset> const &parent,
    Filesystem *filesystem,
    Options const &options,
    Compile::Output *compileOutput,
    Result *result)
{
    bool success = true;

    for (auto const &asset : assets) {
        if (!CompileAsset(asset, parent, filesystem, options, compileOutput, result)) {
            success = false;
        }
    }

    return success;
}

static bool
CompileAsset(
    std::shared_ptr<xcassets::Asset::Asset> const &asset,
    std::shared_ptr<xcassets::Asset::Asset> const &parent,
    Filesystem *filesystem,
    Options const &options,
    Compile::Output *compileOutput,
    Result *result)
{
    std::string filename = FSUtil::GetBaseName(asset->path());
    std::string name = FSUtil::GetBaseNameWithoutExtension(filename);
    switch (asset->type()) {
        case xcassets::Asset::AssetType::AppIconSet: {
            auto appIconSet = std::static_pointer_cast<xcassets::Asset::AppIconSet>(asset);
            if (appIconSet->name().name() == options.appIcon()) {
                Compile::AppIconSet::Compile(appIconSet, filesystem, compileOutput, result);
            }
            break;
        }
        case xcassets::Asset::AssetType::BrandAssets: {
            auto brandAssets = std::static_pointer_cast<xcassets::Asset::BrandAssets>(asset);
            Compile::BrandAssets::Compile(brandAssets, filesystem, compileOutput, result);
            CompileChildren(brandAssets->children(), asset, filesystem, options, compileOutput, result);
            break;
        }
        case xcassets::Asset::AssetType::Catalog: {
            auto catalog = std::static_pointer_cast<xcassets::Asset::Catalog>(asset);
            CompileChildren(catalog->children(), asset, filesystem, options, compileOutput, result);
            break;
        }
        case xcassets::Asset::AssetType::ComplicationSet: {
            auto complicationSet = std::static_pointer_cast<xcassets::Asset::ComplicationSet>(asset);
            Compile::ComplicationSet::Compile(complicationSet, filesystem, compileOutput, result);
            CompileChildren(complicationSet->children(), asset, filesystem, options, compileOutput, result);
            break;
        }
        case xcassets::Asset::AssetType::DataSet: {
            auto dataSet = std::static_pointer_cast<xcassets::Asset::DataSet>(asset);
            Compile::DataSet::Compile(dataSet, filesystem, compileOutput, result);
            break;
        }
        case xcassets::Asset::AssetType::GCDashboardImage: {
            auto dashboardImage = std::static_pointer_cast<xcassets::Asset::GCDashboardImage>(asset);
            Compile::GCDashboardImage::Compile(dashboardImage, filesystem, compileOutput, result);
            CompileChildren(dashboardImage->children(), asset, filesystem, options, compileOutput, result);
            break;
        }
        case xcassets::Asset::AssetType::GCLeaderboard: {
            auto leaderboard = std::static_pointer_cast<xcassets::Asset::GCLeaderboard>(asset);
            Compile::GCLeaderboard::Compile(leaderboard, filesystem, compileOutput, result);
            CompileChildren(leaderboard->children(), asset, filesystem, options, compileOutput, result);
            break;
        }
        case xcassets::Asset::AssetType::GCLeaderboardSet: {
            auto leaderboardSet = std::static_pointer_cast<xcassets::Asset::GCLeaderboardSet>(asset);
            Compile::GCLeaderboardSet::Compile(leaderboardSet, filesystem, compileOutput, result);
            CompileChildren(leaderboardSet->children(), asset, filesystem, options, compileOutput, result);
            break;
        }
        case xcassets::Asset::AssetType::Group: {
            auto group = std::static_pointer_cast<xcassets::Asset::Group>(asset);
            CompileChildren(group->children(), asset, filesystem, options, compileOutput, result);
            break;
        }
        case xcassets::Asset::AssetType::IconSet: {
            auto iconSet = std::static_pointer_cast<xcassets::Asset::IconSet>(asset);
            Compile::IconSet::Compile(iconSet, filesystem, compileOutput, result);
            break;
        }
        case xcassets::Asset::AssetType::ImageSet: {
            auto imageSet = std::static_pointer_cast<xcassets::Asset::ImageSet>(asset);
            Compile::ImageSet::Compile(imageSet, filesystem, compileOutput, result);
            break;
        }
        case xcassets::Asset::AssetType::ImageStack: {
            auto imageStack = std::static_pointer_cast<xcassets::Asset::ImageStack>(asset);
            Compile::ImageStack::Compile(imageStack, filesystem, compileOutput, result);
            CompileChildren(imageStack->children(), asset, filesystem, options, compileOutput, result);
            break;
        }
        case xcassets::Asset::AssetType::ImageStackLayer: {
            auto imageStackLayer = std::static_pointer_cast<xcassets::Asset::ImageStackLayer>(asset);
            Compile::ImageStackLayer::Compile(imageStackLayer, filesystem, compileOutput, result);
            // TODO: CompileChildren(imageStackLayer->children(), asset, filesystem, options, compileOutput, result);
            break;
        }
        case xcassets::Asset::AssetType::LaunchImage: {
            auto launchImage = std::static_pointer_cast<xcassets::Asset::LaunchImage>(asset);
            if (launchImage->name().name() == options.launchImage()) {
                Compile::LaunchImage::Compile(launchImage, filesystem, compileOutput, result);
            }
            break;
        }
        case xcassets::Asset::AssetType::SpriteAtlas: {
            auto spriteAtlas = std::static_pointer_cast<xcassets::Asset::SpriteAtlas>(asset);
            Compile::SpriteAtlas::Compile(spriteAtlas, filesystem, compileOutput, result);
            CompileChildren(spriteAtlas->children(), asset, filesystem, options, compileOutput, result);
            break;
        }
    }

    return true;
}

static bool
WriteOutput(Filesystem *filesystem, Options const &options, Compile::Output const &compileOutput, Output *output, Result *result)
{
    bool success = true;

    /*
     * Collect all inputs and outputs.
     */
    auto info = dependency::DependencyInfo(compileOutput.inputs(), compileOutput.outputs());

    /*
     * Write out compiled archive.
     */
    if (compileOutput.car()) {
        // TODO: only write if non-empty. but did mmap already create the file?
        compileOutput.car()->write();
    }

    /*
     * Copy files into output.
     */
    for (std::pair<std::string, std::string> const &copy : compileOutput.copies()) {
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
    if (options.outputPartialInfoPlist()) {
        auto format = plist::Format::XML::Create(plist::Format::Encoding::UTF8);
        auto serialize = plist::Format::XML::Serialize(compileOutput.additionalInfo(), format);
        if (serialize.first == nullptr) {
            result->normal(Result::Severity::Error, "unable to serialize partial info plist");
            success = false;
        } else {
            if (!filesystem->write(*serialize.first, *options.outputPartialInfoPlist())) {
                result->normal(Result::Severity::Error, "unable to write partial info plist");
                success = false;
            }
        }

        /* Note output file. */
        info.outputs().push_back(*options.outputPartialInfoPlist());
    }

    /*
     * Write out dependency info, if requested.
     */
    if (options.exportDependencyInfo()) {
        auto binaryInfo = dependency::BinaryDependencyInfo();
        binaryInfo.version() = "actool-" + std::to_string(Version::BuildVersion());
        binaryInfo.dependencyInfo() = info;

        if (!filesystem->write(binaryInfo.serialize(), *options.exportDependencyInfo())) {
            result->normal(Result::Severity::Error, "unable to write dependency info");
            success = false;
        }
    }

    /*
     * Add output files to output.
     */
    {
        std::string text;
        auto array = plist::Array::New();

        for (std::string const &output : info.outputs()) {
            /* Array is one entry per file. */
            array->append(plist::String::New(output));

            /* Text is one line per file. */
            text += output;
            text += "\n";
        }

        auto dict = plist::Dictionary::New();
        dict->set("output-files", std::move(array));

        output->add("com.apple.actool.compilation-results", std::move(dict), text);
    }

    return success;
}

static ext::optional<Compile::Output::Format>
DetermineOutputFormat(ext::optional<std::string> const &minimumDeploymentTarget)
{
    if (minimumDeploymentTarget) {
        // TODO: if < 7, use Folder output format
    }

    return Compile::Output::Format::Compiled;
}

void CompileAction::
run(Filesystem *filesystem, Options const &options, Output *output, Result *result)
{
    /*
     * Determine format to output compiled assets.
     */
    ext::optional<Compile::Output::Format> outputFormat = DetermineOutputFormat(options.minimumDeploymentTarget());
    if (!outputFormat) {
        result->normal(Result::Severity::Error, "invalid minimum deployment target");
        return;
    }

    Compile::Output compileOutput = Compile::Output(*options.compile(), *outputFormat);

    /*
     * If necessary, create output archive to write into.
     */
    if (compileOutput.format() == Compile::Output::Format::Compiled) {
        std::string path = compileOutput.root() + "/" + "Assets.car";

        struct bom_context_memory memory = bom_context_memory_file(path.c_str(), true, 0);
        if (memory.data == NULL) {
            result->normal(Result::Severity::Error, "unable to open output for writing");
            return;
        }

        auto bom = car::Writer::unique_ptr_bom(bom_alloc_empty(memory), bom_free);
        if (bom == nullptr) {
            result->normal(Result::Severity::Error, "unable to create output structure");
            return;
        }

        compileOutput.car() = car::Writer::Create(std::move(bom));
        // TODO: should only be an output if ultimately non-empty
        compileOutput.outputs().push_back(path);
    }

    /*
     * Compile each asset catalog into the output.
     */
    for (std::string const &input : options.inputs()) {
        /*
         * Load the input asset catalog.
         */
        auto catalog = xcassets::Asset::Catalog::Load(filesystem, input);
        if (catalog == nullptr) {
            result->normal(
                Result::Severity::Error,
                "unable to load asset catalog",
                ext::nullopt,
                input);
            continue;
        }

        compileOutput.inputs().push_back(input);

        if (!CompileAsset(catalog, catalog, filesystem, options, &compileOutput, result)) {
            /* Error already printed. */
            continue;
        }

        // TODO: use these options:
        /*
        if (arg == "--optimization") {
            return libutil::Options::Next<std::string>(&_optimization, args, it);
        } else if (arg == "--compress-pngs") {
            return libutil::Options::Current<bool>(&_compressPNGs, arg);
        } else if (arg == "--platform") {
            return libutil::Options::Next<std::string>(&_platform, args, it);
        } else if (arg == "--target-device") {
            return libutil::Options::Next<std::string>(&_targetDevice, args, it);
        } else if (arg == "--enable-on-demand-resources") {
            return libutil::Options::Current<bool>(&_enableOnDemandResources, arg);
        } else if (arg == "--enable-incremental-distill") {
            return libutil::Options::Current<bool>(&_enableIncrementalDistill, arg);
        } else if (arg == "--target-name") {
            return libutil::Options::Next<std::string>(&_targetName, args, it);
        } else if (arg == "--filter-for-device-model") {
            return libutil::Options::Next<std::string>(&_filterForDeviceModel, args, it);
        } else if (arg == "--filter-for-device-os-version") {
            return libutil::Options::Next<std::string>(&_filterForDeviceOsVersion, args, it);
        }
        */
    }

    /*
     * Write out the output.
     */
    if (!WriteOutput(filesystem, options, compileOutput, output, result)) {
        /* Error already reported. */
    }
}

