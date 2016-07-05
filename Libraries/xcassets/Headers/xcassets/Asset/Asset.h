/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef __xcassets_Asset_Asset_h
#define __xcassets_Asset_Asset_h

#include <xcassets/Asset/AssetType.h>
#include <xcassets/FullyQualifiedName.h>
#include <plist/Dictionary.h>

#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <ext/optional>

namespace libutil { class Filesystem; }

namespace xcassets {
namespace Asset {

class Asset {
private:
    FullyQualifiedName         _name;
    std::string                _path;
    libutil::Filesystem const *_filesystem;

private:
    ext::optional<std::string> _author;
    ext::optional<int>         _version;

protected:
    Asset(FullyQualifiedName const &name, std::string const &path);
    virtual ~Asset();

public:
    /*
     * The dynamic type of the asset.
     */
    virtual AssetType type() = 0;

public:
    /*
     * The path to the asset.
     */
    std::string const &path()
    { return _path; }

    /*
     * The name of the asset.
     */
    FullyQualifiedName const &name()
    { return _name; }

    libutil::Filesystem const *filesystem()
    { return _filesystem; }

public:
    ext::optional<std::string> const &author()
    { return _author; }
    ext::optional<int> const &version()
    { return _version; }

public:
    /*
     * Load an asset from a directory.
     */
    static std::shared_ptr<Asset> Load(
        libutil::Filesystem const *filesystem,
        std::string const &path,
        std::vector<std::string> const &groups,
        ext::optional<std::string> const &overrideExtension = ext::nullopt);

protected:
    /*
     * Load the asset from the filesystem. Default implementation calls parse; override to load children.
     */
    virtual bool load(libutil::Filesystem const *filesystem);

    /*
     * Override to parse the contents, which can be null.
     */
    virtual bool parse(plist::Dictionary const *dict, std::unordered_set<std::string> *seen, bool check);

protected:
    /*
     * Without loading, checks for child assets.
     */
    bool hasChildren(libutil::Filesystem const *filesystem);

    /*
     * Iterate children of this asset and load them.
     */
    bool loadChildren(libutil::Filesystem const *filesystem, std::vector<std::shared_ptr<Asset>> *children, bool providesNamespace = false);

    /*
     * Load children of a specific type.
     */
    template<typename T>
    bool loadChildren(libutil::Filesystem const *filesystem, std::vector<std::shared_ptr<T>> *children, bool providesNamespace = false)
    {
        std::vector<std::shared_ptr<Asset>> assets;
        if (!loadChildren(filesystem, &assets)) {
            return false;
        }

        bool error = false;
        for (std::shared_ptr<Asset> const &asset : assets) {
            if (asset->type() == T::Type()) {
                auto child = std::static_pointer_cast<T>(asset);
                children->push_back(child);
            } else {
                error = true;
            }
        }
        return !error;
    }
};

}
}

#endif // !__xcassets_Asset_Asset_h

