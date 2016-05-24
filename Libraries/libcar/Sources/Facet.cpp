/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#include <car/Facet.h>
#include <car/AttributeList.h>
#include <car/Rendition.h>
#include <car/Reader.h>

#include <cstring>
#include <cstdlib>

using car::Facet;

Facet::
Facet(std::string const &name, AttributeList const &attributes) :
    _name      (name),
    _attributes(attributes)
{
}

void Facet::
dump() const
{
    fprintf(stderr, "Facet: %s\n", _name.c_str());

    ext::optional<car::AttributeList> attributes = this->attributes();
    if (attributes) {
        attributes->dump();
    }
}

void Facet::
renditionIterate(Reader const *archive, std::function<void(Rendition const &)> const &iterator) const
{
    ext::optional<car::AttributeList> attributes = this->attributes();
    if (!attributes) {
        return;
    }

    ext::optional<uint16_t> facet_identifier = attributes->get(car_attribute_identifier_identifier);
    if (facet_identifier) {
        archive->renditionIterate([&facet_identifier, &iterator](Rendition const &rendition) {
            ext::optional<uint16_t> rendition_identifier = rendition.attributes().get(car_attribute_identifier_identifier);
            if (rendition_identifier && *rendition_identifier == *facet_identifier) {
                iterator(rendition);
            }
        });
    }
}

Facet Facet::
Load(
    std::string const &name,
    struct car_facet_value *value)
{
    AttributeList attributes = car::AttributeList::Load(value->attributes_count, value->attributes);
    return Facet(name, attributes);
}

Facet Facet::
Create(std::string const &name, AttributeList const &attributes)
{
    return Facet(name, attributes);
}

