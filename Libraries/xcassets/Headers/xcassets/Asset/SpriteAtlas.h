/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef __xcassets_Asset_SpriteAtlas_h
#define __xcassets_Asset_SpriteAtlas_h

#include <xcassets/Asset/Asset.h>
#include <xcassets/Compression.h>
#include <plist/Dictionary.h>

#include <memory>
#include <string>
#include <vector>
#include <ext/optional>

namespace xcassets {
namespace Asset {

class ImageSet;

class SpriteAtlas : public Asset {
private:
    std::vector<std::shared_ptr<ImageSet>>  _children;

private:
    ext::optional<Compression>              _compression;
    ext::optional<std::vector<std::string>> _onDemandResourceTags;
    ext::optional<bool>                     _providesNamespace;

private:
    friend class Asset;
    using Asset::Asset;

public:
    std::vector<std::shared_ptr<ImageSet>> const &children() const
    { return _children; }

public:
    ext::optional<Compression> const &compression() const
    { return _compression; }
    ext::optional<std::vector<std::string>> const &onDemandResourceTags() const
    { return _onDemandResourceTags; }
    bool providesNamespace() const
    { return _providesNamespace.value_or(false); }
    ext::optional<bool> providesNamespaceOptional() const
    { return _providesNamespace; }

public:
    static AssetType Type()
    { return AssetType::SpriteAtlas; }
    virtual AssetType type()
    { return AssetType::SpriteAtlas; }

public:
    static ext::optional<std::string> Extension()
    { return std::string("spriteatlas"); }

protected:
    virtual bool load(libutil::Filesystem const *filesystem);
    virtual bool parse(plist::Dictionary const *dict, std::unordered_set<std::string> *seen, bool check);
};

}
}

#endif // !__xcassets_Asset_SpriteAtlas_h
