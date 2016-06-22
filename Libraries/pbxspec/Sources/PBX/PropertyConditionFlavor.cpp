/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#include <pbxspec/PBX/PropertyConditionFlavor.h>
#include <pbxspec/Inherit.h>
#include <plist/Dictionary.h>
#include <plist/Integer.h>
#include <plist/Keys/Unpack.h>

using pbxspec::PBX::PropertyConditionFlavor;

PropertyConditionFlavor::
PropertyConditionFlavor() :
    Specification()
{
}

PropertyConditionFlavor::
~PropertyConditionFlavor()
{
}

PropertyConditionFlavor::shared_ptr PropertyConditionFlavor::
Parse(Context *context, plist::Dictionary const *dict)
{
    if (!ParseType(context, dict, Type())) {
        return nullptr;
    }

    PropertyConditionFlavor::shared_ptr result;
    result.reset(new PropertyConditionFlavor());

    std::unordered_set<std::string> seen;
    if (!result->parse(context, dict, &seen, true))
        return nullptr;

    return result;
}

bool PropertyConditionFlavor::
parse(Context *context, plist::Dictionary const *dict, std::unordered_set<std::string> *seen, bool check)
{
    if (!Specification::parse(context, dict, seen, false))
        return false;

    auto unpack = plist::Keys::Unpack("PropertyConditionFlavor", dict, seen);

    auto P = unpack.coerce <plist::Integer> ("Precedence");

    if (!unpack.complete(check)) {
        fprintf(stderr, "%s", unpack.errorText().c_str());
    }

    if (P != nullptr) {
        _precedence = P->value();
    }

    return true;
}

bool PropertyConditionFlavor::
inherit(Specification::shared_ptr const &base)
{
    if (base->type() != PropertyConditionFlavor::Type())
        return false;

    return inherit(std::static_pointer_cast<PropertyConditionFlavor>(base));
}

bool PropertyConditionFlavor::
inherit(PropertyConditionFlavor::shared_ptr const &b)
{
    if (!Specification::inherit(b))
        return false;

    auto base = this->base();

    _precedence = Inherit::Override(_precedence, base->_precedence);

    return true;
}
