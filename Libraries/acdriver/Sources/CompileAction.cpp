/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#include <acdriver/CompileAction.h>
#include <acdriver/CompileOutput.h>
#include <acdriver/Options.h>
#include <acdriver/Output.h>
#include <acdriver/Result.h>
#include <dependency/BinaryDependencyInfo.h>
#include <car/Writer.h>
#include <libutil/Filesystem.h>
#include <plist/Dictionary.h>
#include <xcassets/Asset/AppIconSet.h>
#include <xcassets/Asset/BrandAssets.h>
#include <xcassets/Asset/Catalog.h>
#include <xcassets/Asset/ComplicationSet.h>
#include <xcassets/Asset/DataSet.h>
#include <xcassets/Asset/GCDashboardImage.h>
#include <xcassets/Asset/GCLeaderboard.h>
#include <xcassets/Asset/GCLeaderboardSet.h>
#include <xcassets/Asset/Group.h>
#include <xcassets/Asset/IconSet.h>
#include <xcassets/Asset/ImageSet.h>
#include <xcassets/Asset/ImageStack.h>
#include <xcassets/Asset/ImageStackLayer.h>
#include <xcassets/Asset/LaunchImage.h>
#include <xcassets/Asset/SpriteAtlas.h>

using acdriver::CompileAction;
using acdriver::CompileOutput;
using acdriver::Options;
using acdriver::Output;
using acdriver::Result;
using libutil::Filesystem;

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
    std::shared_ptr<xcassets::Asset::Catalog> const &catalog,
    Options const &options,
    CompileOutput *compileOutput,
    Result *result,
    std::shared_ptr<xcassets::Asset::Asset> const &asset);

template<typename T>
static bool
CompileChildren(
    std::shared_ptr<xcassets::Asset::Catalog> const &catalog,
    Options const &options,
    CompileOutput *compileOutput,
    Result *result,
    std::vector<T> const &assets)
{
    bool success = true;

    for (auto const &asset : assets) {
        if (!CompileAsset(catalog, options, compileOutput, result, asset)) {
            success = false;
        }
    }

    return success;
}

static std::string
AssetReference(std::shared_ptr<xcassets::Asset::Asset> const &asset)
{
    // TODO: include [] for each key
    return asset->path();
}

static void
CompileAppIconSet(std::shared_ptr<xcassets::Asset::AppIconSet> const &appIconSet, CompileOutput *compileOutput, Result *result)
{
    // TODO: what should this do for watch?

    auto primary = plist::Dictionary::New();

    /*
     * Copy the app icon images into the output.
     */
    if (appIconSet->images()) {
        auto files = plist::Array::New();

        for (xcassets::Asset::AppIconSet::Image const &image : *appIconSet->images()) {
            if (!image.fileName()) {
                continue;
            }

            // TODO: verify the dimensions of the image are correct

            // TODO: how should these be named?
            std::string source = appIconSet->path() + "/" + *image.fileName();
            std::string destination = compileOutput->root() + "/" + appIconSet->name().name();
            // TODO: add WIDTHxHEIGHT
            // TODO: add @SCALEx (if not 1)
            // TODO: add ~idiom (if not phone)

            compileOutput->copies().push_back({ source, destination });

            // TODO: what should the value in this array be?
            files->append(plist::String::New(*image.fileName()));
        }

        primary->set("CFBundleIconFiles", std::move(files));
    }

    /*
     * Record if the icon is pre-rendered.
     */
    if (appIconSet->preRendered()) {
        primary->set("UIPrerenderedIcon", plist::Boolean::New(true));
    }

    /*
     * Store the icon information in the Info.plist.
     */
    auto icons = plist::Dictionary::New();
    icons->set("CFBundlePrimaryIcon", std::move(primary));

    // TODO: needs a separate case for ~ipad
    compileOutput->additionalInfo()->set("CFBundleIcons", std::move(icons));
}

static bool
CompileAsset(
    std::shared_ptr<xcassets::Asset::Catalog> const &catalog,
    Options const &options,
    CompileOutput *compileOutput,
    Result *result,
    std::shared_ptr<xcassets::Asset::Asset> const &asset)
{
    switch (asset->type()) {
        case xcassets::Asset::AssetType::AppIconSet: {
            auto appIconSet = std::static_pointer_cast<xcassets::Asset::AppIconSet>(asset);
            if (appIconSet->name().name() == options.appIcon()) {
                CompileAppIconSet(appIconSet, compileOutput, result);
            }
            break;
        }
        case xcassets::Asset::AssetType::BrandAssets: {
            auto brandAssets = std::static_pointer_cast<xcassets::Asset::BrandAssets>(asset);
            result->document(Result::Severity::Warning, catalog->path(), { AssetReference(asset) }, "Not Implemented", "brand assets not yet supported");
            CompileChildren(catalog, options, compileOutput, result, brandAssets->children());
            break;
        }
        case xcassets::Asset::AssetType::Catalog: {
            auto catalog = std::static_pointer_cast<xcassets::Asset::Catalog>(asset);
            CompileChildren(catalog, options, compileOutput, result, catalog->children());
            break;
        }
        case xcassets::Asset::AssetType::ComplicationSet: {
            auto complicationSet = std::static_pointer_cast<xcassets::Asset::ComplicationSet>(asset);
            result->document(Result::Severity::Warning, catalog->path(), { AssetReference(asset) }, "Not Implemented", "complication set not yet supported");
            CompileChildren(catalog, options, compileOutput, result, complicationSet->children());
            break;
        }
        case xcassets::Asset::AssetType::DataSet: {
            auto dataSet = std::static_pointer_cast<xcassets::Asset::DataSet>(asset);
            result->document(Result::Severity::Warning, catalog->path(), { AssetReference(asset) }, "Not Implemented", "data set not yet supported");
            break;
        }
        case xcassets::Asset::AssetType::GCDashboardImage: {
            auto dashboardImage = std::static_pointer_cast<xcassets::Asset::GCDashboardImage>(asset);
            result->document(Result::Severity::Warning, catalog->path(), { AssetReference(asset) }, "Not Implemented", "gc dashboard image not yet supported");
            CompileChildren(catalog, options, compileOutput, result, dashboardImage->children());
            break;
        }
        case xcassets::Asset::AssetType::GCLeaderboard: {
            auto leaderboard = std::static_pointer_cast<xcassets::Asset::GCLeaderboard>(asset);
            result->document(Result::Severity::Warning, catalog->path(), { AssetReference(asset) }, "Not Implemented", "gc leaderboard not yet supported");
            CompileChildren(catalog, options, compileOutput, result, leaderboard->children());
            break;
        }
        case xcassets::Asset::AssetType::GCLeaderboardSet: {
            auto leaderboardSet = std::static_pointer_cast<xcassets::Asset::GCLeaderboardSet>(asset);
            result->document(Result::Severity::Warning, catalog->path(), { AssetReference(asset) }, "Not Implemented", "gc leaderboard set not yet supported");
            CompileChildren(catalog, options, compileOutput, result, leaderboardSet->children());
            break;
        }
        case xcassets::Asset::AssetType::Group: {
            auto group = std::static_pointer_cast<xcassets::Asset::Group>(asset);
            CompileChildren(catalog, options, compileOutput, result, group->children());
            break;
        }
        case xcassets::Asset::AssetType::IconSet: {
            auto iconSet = std::static_pointer_cast<xcassets::Asset::IconSet>(asset);
            result->document(Result::Severity::Warning, catalog->path(), { AssetReference(asset) }, "Not Implemented", "icon set not yet supported");
            break;
        }
        case xcassets::Asset::AssetType::ImageSet: {
            auto imageSet = std::static_pointer_cast<xcassets::Asset::ImageSet>(asset);
            result->document(Result::Severity::Warning, catalog->path(), { AssetReference(asset) }, "Not Implemented", "image set not yet supported");
            break;
        }
        case xcassets::Asset::AssetType::ImageStack: {
            auto imageStack = std::static_pointer_cast<xcassets::Asset::ImageStack>(asset);
            result->document(Result::Severity::Warning, catalog->path(), { AssetReference(asset) }, "Not Implemented", "image stack not yet supported");
            CompileChildren(catalog, options, compileOutput, result, imageStack->children());
            break;
        }
        case xcassets::Asset::AssetType::ImageStackLayer: {
            auto imageStackLayer = std::static_pointer_cast<xcassets::Asset::ImageStackLayer>(asset);
            result->document(Result::Severity::Warning, catalog->path(), { AssetReference(asset) }, "Not Implemented", "image stack layer not yet supported");
            // TODO: CompileChildren(catalog, options, compileOutput, result, imageStackLayer->children());
            break;
        }
        case xcassets::Asset::AssetType::LaunchImage: {
            auto launchImage = std::static_pointer_cast<xcassets::Asset::LaunchImage>(asset);
            if (launchImage->name().name() == options.launchImage()) {

            }
            result->document(Result::Severity::Warning, catalog->path(), { AssetReference(asset) }, "Not Implemented", "launch image not yet supported");
            break;
        }
        case xcassets::Asset::AssetType::SpriteAtlas: {
            auto spriteAtlas = std::static_pointer_cast<xcassets::Asset::SpriteAtlas>(asset);
            result->document(Result::Severity::Warning, catalog->path(), { AssetReference(asset) }, "Not Implemented", "sprite atlas not yet supported");
            CompileChildren(catalog, options, compileOutput, result, spriteAtlas->children());
            break;
        }
    }

    return true;
}

static ext::optional<CompileOutput::Format>
DetermineOutputFormat(std::string const &minimumDeploymentTarget)
{
    if (!minimumDeploymentTarget.empty()) {
        // TODO: if < 7, use Folder output format
    }

    return CompileOutput::Format::Compiled;
}

void CompileAction::
run(Filesystem *filesystem, Options const &options, Output *output, Result *result)
{
    /*
     * Determine format to output compiled assets.
     */
    ext::optional<CompileOutput::Format> outputFormat = DetermineOutputFormat(options.minimumDeploymentTarget());
    if (!outputFormat) {
        result->normal(Result::Severity::Error, "invalid minimum deployment target");
        return;
    }

    CompileOutput compileOutput = CompileOutput(options.compile(), *outputFormat);

    /*
     * If necssary, create output archive to write into.
     */
    if (compileOutput.format() == CompileOutput::Format::Compiled) {
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

        if (!CompileAsset(catalog, options, &compileOutput, result, catalog)) {
            /* Error already printed. */
            continue;
        }

        // TODO: use these options:
        /*
        if (arg == "--optimization") {
            return libutil::Options::NextString(&_optimization, args, it);
        } else if (arg == "--compress-pngs") {
            return libutil::Options::MarkBool(&_compressPNGs, arg);
        } else if (arg == "--platform") {
            return libutil::Options::NextString(&_platform, args, it);
        } else if (arg == "--target-device") {
            return libutil::Options::NextString(&_targetDevice, args, it);
        } else if (arg == "--app-icon") {
            return libutil::Options::NextString(&_appIcon, args, it);
        } else if (arg == "--launch-image") {
            return libutil::Options::NextString(&_launchImage, args, it);
        } else if (arg == "--enable-on-demand-resources") {
            return libutil::Options::MarkBool(&_enableOnDemandResources, arg);
        } else if (arg == "--enable-incremental-distill") {
            return libutil::Options::MarkBool(&_enableIncrementalDistill, arg);
        } else if (arg == "--target-name") {
            return libutil::Options::NextString(&_targetName, args, it);
        } else if (arg == "--filter-for-device-model") {
            return libutil::Options::NextString(&_filterForDeviceModel, args, it);
        } else if (arg == "--filter-for-device-os-version") {
            return libutil::Options::NextString(&_filterForDeviceOsVersion, args, it);
        }
        */
    }

    /*
     * Write out the output.
     */
    if (!compileOutput.write(filesystem, options, result)) {
        /* Error already reported. */
    }
}

