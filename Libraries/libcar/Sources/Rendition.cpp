/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#include <car/Rendition.h>
#include <car/Reader.h>
#include <car/car_format.h>

#include <cassert>
#include <cstring>
#include <cstdio>

#include <zlib.h>

#if defined(__APPLE__)
#include <Availability.h>
#include <TargetConditionals.h>
#if (TARGET_OS_MAC && __MAC_10_11 && __MAC_OS_X_VERSION_MIN_REQUIRED > __MAC_10_11) || (TARGET_OS_IPHONE && __IPHONE_9_0 && __IPHONE_OS_VERSION_MIN_REQUIRED > __IPHONE_9_0)
#define HAVE_LIBCOMPRESSION 1
#endif
#endif

#if HAVE_LIBCOMPRESSION
#include <compression.h>
#define _COMPRESSION_LZVN 0x900
#endif

using car::Rendition;

Rendition::Data::
Data(std::vector<uint8_t> const &data, Format format) :
    _data  (data),
    _format(format)
{
}

size_t Rendition::Data::
FormatSize(Rendition::Data::Format format)
{
    switch (format) {
        case Format::PremultipliedBGRA8:
            return 4;
        case Format::PremultipliedGA8:
            return 2;
        case Format::Data:
            return 1;
    }

    abort();
}

Rendition::
Rendition(AttributeList const &attributes, std::function<ext::optional<Data>(Rendition const *)> const &data) :
    _attributes(attributes),
    _deferredData (data),
    _width     (0),
    _height    (0),
    _scale     (1.0),
    _is_vector (false),
    _is_opaque (false),
    _is_resizable (false)
{
}

Rendition::
Rendition(AttributeList const &attributes, ext::optional<Data> const &data) :
    _attributes(attributes),
    _data (data),
    _width     (0),
    _height    (0),
    _scale     (1.0),
    _is_vector (false),
    _is_opaque (false),
    _is_resizable (false)
{
}

void Rendition::
dump() const
{
    printf("Rendition: %s\n", _fileName.c_str());
    printf("Width: %d\n", _width);
    printf("Height: %d\n", _height);
    printf("Scale: %f\n", _scale);
    printf("Layout: %d\n", _layout);

    printf("Resizable: %d\n", _is_resizable);
    if (_is_resizable) {
        int i = 0;
        for (auto slice : _slices) {
            printf("slice %d (%u, %u) %u x %u \n", i++, slice.x, slice.y, slice.width, slice.height);
        }
    }

    switch(_resize_mode) {
        case resize_mode_fixed_size:
            printf("Resize mode: resize_mode_fixed_size\n");
            break;
        case resize_mode_tile:
            printf("Resize mode: resize_mode_tile\n");
            break;
        case resize_mode_scale:
            printf("Resize mode: resize_mode_scale\n");
            break;
        case resize_mode_uniform:
            printf("Resize mode: resize_mode_uniform\n");
            break;
        case resize_mode_horizontal_uniform_vertical_scale:
            printf("Resize mode: resize_mode_horizontal_uniform_vertical_scale\n");
            break;
        case resize_mode_horizontal_scale_vertical_uniform:
            printf("Resize mode: resize_mode_horizontal_scale_vertical_uniform\n");
            break;
    }

    printf("Attributes:\n");
    _attributes.dump();
}

static ext::optional<Rendition::Data> _decode(struct car_rendition_value *value);
static ext::optional<std::vector<uint8_t>> _encode(Rendition *rendition);

static int number_slices_from_layout(enum car_rendition_value_layout layout)
{
    switch(layout) {
    case car_rendition_value_layout_one_part_fixed_size:
    case car_rendition_value_layout_one_part_tile:
    case car_rendition_value_layout_one_part_scale:
        return 1;

    case car_rendition_value_layout_three_part_horizontal_tile:
    case car_rendition_value_layout_three_part_horizontal_scale:
    case car_rendition_value_layout_three_part_horizontal_uniform:
    case car_rendition_value_layout_three_part_vertical_tile:
    case car_rendition_value_layout_three_part_vertical_scale:
    case car_rendition_value_layout_three_part_vertical_uniform:
        return 3;

    case car_rendition_value_layout_nine_part_tile:
    case car_rendition_value_layout_nine_part_scale:
    case car_rendition_value_layout_nine_part_horizontal_uniform_vertical_scale:
    case car_rendition_value_layout_nine_part_horizontal_scale_vertical_uniform:
        return 9;

    case car_rendition_value_layout_six_part:
        return 6;

    case car_rendition_value_layout_gradient:
    case car_rendition_value_layout_effect:
    case car_rendition_value_layout_animation_filmstrip:
    case car_rendition_value_layout_raw_data:
    case car_rendition_value_layout_external_link:
    case car_rendition_value_layout_layer_stack:
    case car_rendition_value_layout_internal_link:
    case car_rendition_value_layout_asset_pack:
        break;
    }
    return 0;
}

static enum car::Rendition::resize_mode resize_mode_from_layout(enum car_rendition_value_layout layout)
{
    switch(layout) {
    case car_rendition_value_layout_one_part_fixed_size:
    case car_rendition_value_layout_three_part_horizontal_uniform:
    case car_rendition_value_layout_three_part_vertical_uniform:
        return car::Rendition::resize_mode_fixed_size;
    case car_rendition_value_layout_one_part_tile:
    case car_rendition_value_layout_three_part_horizontal_tile:
    case car_rendition_value_layout_three_part_vertical_tile:
    case car_rendition_value_layout_nine_part_tile:
        return car::Rendition::resize_mode_tile;
    case car_rendition_value_layout_one_part_scale:
    case car_rendition_value_layout_three_part_horizontal_scale:
    case car_rendition_value_layout_three_part_vertical_scale:
    case car_rendition_value_layout_nine_part_scale:
        return car::Rendition::resize_mode_scale;
    case car_rendition_value_layout_nine_part_horizontal_uniform_vertical_scale:
        return car::Rendition::resize_mode_horizontal_uniform_vertical_scale;
    case car_rendition_value_layout_nine_part_horizontal_scale_vertical_uniform:
        return car::Rendition::resize_mode_horizontal_scale_vertical_uniform;
    case car_rendition_value_layout_six_part:
    case car_rendition_value_layout_gradient:
    case car_rendition_value_layout_effect:
    case car_rendition_value_layout_animation_filmstrip:
    case car_rendition_value_layout_raw_data:
    case car_rendition_value_layout_external_link:
    case car_rendition_value_layout_layer_stack:
    case car_rendition_value_layout_internal_link:
    case car_rendition_value_layout_asset_pack:
        break;
    }
    return car::Rendition::resize_mode_fixed_size;
}

Rendition const Rendition::
Load(
    AttributeList const &attributes,
    struct car_rendition_value *value)
{
    Rendition rendition = Rendition(attributes, [value](Rendition const *rendition) -> ext::optional<Data> {
        return _decode(value);
    });

    for (struct car_rendition_info_header *info_header = (struct car_rendition_info_header *)value->info;
        ((uintptr_t)info_header - (uintptr_t)value->info) < value->info_len;
        info_header = (struct car_rendition_info_header *)((intptr_t)info_header + sizeof(struct car_rendition_info_header) + info_header->length)) {
        switch(info_header->magic) {
            case car_rendition_info_magic_slices:
            {
                std::vector<slice> slices;
                int count = number_slices_from_layout((enum car_rendition_value_layout)value->metadata.layout);
                struct car_rendition_info_slices *info_slices = (struct car_rendition_info_slices *)info_header;
                for(int i = 0; i < count; i++) {
                        car::Rendition::slice slice = {info_slices->slices[i].x, info_slices->slices[i].y,
                            info_slices->slices[i].width, info_slices->slices[i].height};
                        slices.push_back(slice);
                }
                rendition.slices() = std::move(slices);
            }
            break;
            case car_rendition_info_magic_metrics:
            {
                // Alignment options
                // struct car_rendition_info_metrics *metric = (struct car_rendition_info_metrics *)info_header;
                // metric->nmetrics
                // metric->top_right_inset.width
                // metric->top_right_inset.height
                // metric->bottom_left_inset.width
                // metric->bottom_left_inset.height
                // metric->image_size.width
                // metric->image_size.height
            }
            break;
            case car_rendition_info_magic_composition:
            {
                // struct car_rendition_info_composition *composition = (struct car_rendition_info_composition *)info_header;
                // composition->blend_mode
                // composition->opacity
            }
            break;
            case car_rendition_info_magic_uti:
            {
                struct car_rendition_info_uti *uti = (struct car_rendition_info_uti *)info_header;
                rendition.uti() = std::string(uti->uti, uti->uti_length);
            }
            break;
            case car_rendition_info_magic_bitmap_info:
            break;
            case car_rendition_info_magic_bytes_per_row:
            {

            }
            break;
            case car_rendition_info_magic_reference:
            {
                // struct car_rendition_info_reference *reference = (struct car_rendition_info_reference *)info_header;
                // reference->x
                // reference->y
                // reference->width
                // reference->height
                // reference->layout
                // reference->key_length
                // reference->key[]
            }
            break;
            case car_rendition_info_magic_alpha_cropped_frame:
                break;
        }

    }

    rendition.fileName() = std::string(value->metadata.name, sizeof(value->metadata.name));
    rendition.width() = value->width;
    rendition.height() = value->height;
    rendition.scale() = (float)value->scale_factor / 100.0;
    rendition.is_vector() = (int)value->flags.is_vector;
    rendition.is_opaque() = (int)value->flags.is_opaque;

    enum car_rendition_value_layout layout = (enum car_rendition_value_layout)value->metadata.layout;
    rendition.layout() = layout;
    rendition.resize_mode() = resize_mode_from_layout(layout);

    if (layout >= car_rendition_value_layout_three_part_horizontal_tile &&
        layout <= car_rendition_value_layout_nine_part_horizontal_scale_vertical_uniform &&
        rendition.slices().size() > 0) {
        rendition.is_resizable() = true;
    }
    return rendition;
}

ext::optional<Rendition::Data> Rendition::
data() const
{
    if(_data) {
        return _data;
    }

    if (_deferredData) {
        return _deferredData(this);
    }

    return ext::nullopt;
}

static ext::optional<Rendition::Data>
_decode(struct car_rendition_value *value)
{
    Rendition::Data::Format format;
    if (value->pixel_format == car_rendition_value_pixel_format_argb) {
        format = Rendition::Data::Format::PremultipliedBGRA8;
    } else if (value->pixel_format == car_rendition_value_pixel_format_ga8) {
        format = Rendition::Data::Format::PremultipliedGA8;
    } else {
        format = Rendition::Data::Format::Data;
        fprintf(stderr, "error: unsupported pixel format %.4s\n", (char const *)&value->pixel_format);
        return ext::nullopt;
    }

    size_t bytes_per_pixel = Rendition::Data::FormatSize(format);
    size_t uncompressed_length = value->width * value->height * bytes_per_pixel;
    Rendition::Data data = Rendition::Data(std::vector<uint8_t>(uncompressed_length), format);
    void *uncompressed_data = static_cast<void *>(data.data().data());

    /* Advance past the header and the info section. We just want the data. */
    struct car_rendition_data_header1 *header1 = (struct car_rendition_data_header1 *)((uintptr_t)value + sizeof(struct car_rendition_value) + value->info_len);

    if (strncmp(header1->magic, "MLEC", sizeof(header1->magic)) != 0) {
        fprintf(stderr, "error: header1 magic is wrong, can't possibly decode\n");
        return ext::nullopt;
    }

    void *compressed_data = &header1->data;
    size_t compressed_length = header1->length;

    /* Check for the secondary header, and use its values if available. */
    /* todo find a way of determining in advance if this is present */
    struct car_rendition_data_header2 *header2 = (struct car_rendition_data_header2 *)compressed_data;
    if (strncmp(header2->magic, "KCBC", 4) == 0) {
        compressed_data = &header2->data;
        compressed_length = header2->length;
    }

    size_t offset = 0;
    while (offset < uncompressed_length) {
        if (offset != 0) {
            struct car_rendition_data_header2 *header2 = (struct car_rendition_data_header2 *)compressed_data;
            assert(strncmp(header2->magic, "KCBC", sizeof(header2->magic)) == 0);
            compressed_length = header2->length;
            compressed_data = header2->data;
        }

        if (header1->compression == car_rendition_data_compression_magic_zlib) {
            z_stream strm;
            strm.zalloc = Z_NULL;
            strm.zfree = Z_NULL;
            strm.opaque = Z_NULL;
            strm.avail_in = compressed_length;
            strm.next_in = (Bytef *)compressed_data;

            int ret = inflateInit2(&strm, 16+MAX_WBITS);
            if (ret != Z_OK) {
               return ext::nullopt;
            }

            strm.avail_out = uncompressed_length;
            strm.next_out = (Bytef *)uncompressed_data;

            ret = inflate(&strm, Z_NO_FLUSH);
            if (ret != Z_OK && ret != Z_STREAM_END) {
                printf("error: decompression failure: %x.\n", ret);
                return ext::nullopt;
            }

            ret = inflateEnd(&strm);
            if (ret != Z_OK) {
                return ext::nullopt;
            }

            offset += (uncompressed_length - strm.avail_out);
        } else if (header1->compression == car_rendition_data_compression_magic_rle) {
            fprintf(stderr, "error: unable to handle RLE\n");
            return ext::nullopt;
        } else if (header1->compression == car_rendition_data_compression_magic_unk1) {
            fprintf(stderr, "error: unable to handle UNKNOWN\n");
            return ext::nullopt;
        } else if (header1->compression == car_rendition_data_compression_magic_lzvn || header1->compression == car_rendition_data_compression_magic_jpeg_lzfse) {
#if HAVE_LIBCOMPRESSION
            compression_algorithm algorithm;
            if (header1->compression == car_rendition_data_compression_magic_lzvn) {
                algorithm = (compression_algorithm)_COMPRESSION_LZVN;
            } else if (header1->compression == car_rendition_data_compression_magic_jpeg_lzfse) {
                algorithm = COMPRESSION_LZFSE;
            } else {
                assert(false);
            }

            size_t compression_result = compression_decode_buffer((uint8_t *)uncompressed_data + offset, uncompressed_length - offset, (uint8_t *)compressed_data, compressed_length, NULL, algorithm);
            if (compression_result != 0) {
                offset += compression_result;
                compressed_data = (void *)((uintptr_t)compressed_data + compressed_length);
            } else {
                fprintf(stderr, "error: decompression failure\n");
                return ext::nullopt;
            }
#else
            if (header1->compression == car_rendition_data_compression_magic_lzvn) {
                fprintf(stderr, "error: unable to handle LZVN\n");
                return ext::nullopt;
            } else if (header1->compression == car_rendition_data_compression_magic_jpeg_lzfse) {
                fprintf(stderr, "error: unable to handle LZFSE\n");
                return ext::nullopt;
            } else {
                assert(false);
            }
#endif
        } else if (header1->compression == car_rendition_data_compression_magic_blurredimage) {
            fprintf(stderr, "error: unable to handle BlurredImage\n");
            return ext::nullopt;
        } else {
            fprintf(stderr, "error: unknown compression algorithm %x\n", header1->compression);
            return ext::nullopt;
        }
    }

    return data;
}

static ext::optional<std::vector<uint8_t>>
_encode(Rendition *rendition)
{
    ext::optional<Rendition::Data> data = rendition->data();
    if (!data || data->data().size() == 0) {
        return ext::nullopt;
    }

    // The selected algorithm, only zlib for now
    enum car_rendition_data_compression_magic compression_magic = car_rendition_data_compression_magic_zlib;
    int bytes_per_pixel = 0;
    switch(data->format()) {
        case Rendition::Data::Format::PremultipliedBGRA8:
            bytes_per_pixel = 4;
            break;
        case Rendition::Data::Format::PremultipliedGA8:
            bytes_per_pixel = 2;
            break;
        case Rendition::Data::Format::Data:
            return ext::nullopt;
    }

    size_t uncompressed_length = rendition->width() * rendition->height() * bytes_per_pixel;
    void *uncompressed_data = static_cast<void *>(data->data().data());

    std::vector<uint8_t> compressed_vector;
    if (compression_magic == car_rendition_data_compression_magic_zlib) {
        int deflateLevel = Z_DEFAULT_COMPRESSION;
        int windowSize = 16+MAX_WBITS;
        z_stream zlibStream;
        memset(&zlibStream, 0, sizeof(zlibStream));
        zlibStream.next_in = (Bytef*)uncompressed_data;
        zlibStream.avail_in = (uInt)uncompressed_length;
        int err = deflateInit2(&zlibStream, deflateLevel, Z_DEFLATED, windowSize, 8, Z_DEFAULT_STRATEGY);
        if (err != Z_OK) {
            return ext::nullopt;
        }
        while (true) {
            uint8_t tmp[4096];
            zlibStream.next_out = (Bytef*)&tmp;
            zlibStream.avail_out = (uInt)sizeof(tmp);
            err = deflate(&zlibStream, Z_FINISH);
            size_t block_size = sizeof(tmp) - zlibStream.avail_out;
            compressed_vector.resize(compressed_vector.size() + block_size);
            memcpy(&compressed_vector[compressed_vector.size() - block_size], &tmp[0], block_size);
            if (err == Z_STREAM_END) {  /* Done */
                break;
            }
            if (err != Z_OK) {  /* Z_OK -> Made progress, else err */
                deflateEnd(&zlibStream);
                fprintf(stderr, "Zlib error %d", err);
                return ext::nullopt;
            }
        }
        deflateEnd(&zlibStream);
    }

    std::vector<uint8_t> output = std::vector<uint8_t>(sizeof(struct car_rendition_data_header1));

    struct car_rendition_data_header1 *header1 = (struct car_rendition_data_header1 *) &output[0];
    memcpy(header1->magic, "MLEC", sizeof(header1->magic));
    header1->length = compressed_vector.size();
    header1->compression = compression_magic;
    output.insert(output.end(), compressed_vector.begin(), compressed_vector.end());

    return output;
}

Rendition Rendition::
Create(
    AttributeList const &attributes,
    std::function<ext::optional<Data>(Rendition const *)> const &data)
{
    return Rendition(attributes, data);
}

Rendition Rendition::
Create(
    AttributeList const &attributes,
    ext::optional<Data> const &data)
{
    return Rendition(attributes, data);
}

std::vector<uint8_t> Rendition::Write()
{
    // Create header
    struct car_rendition_value header;
    bzero(&header, sizeof(struct car_rendition_value));
    header.magic[3] = 'C';
    header.magic[2] = 'T';
    header.magic[1] = 'S';
    header.magic[0] = 'I';
    header.version = 1;
    // header.flags.is_header_flagged_fpo = 0;
    // header.flags.is_excluded_from_contrast_filter = 0;
    header.flags.is_vector = _is_vector;
    header.flags.is_opaque = _is_opaque;
    header.flags.bitmap_encoding = 1;
    // header.flags.reserved = 0;

    header.width = _width;
    header.height = _height;
    header.scale_factor = (uint32_t)(_scale * 100);
    header.pixel_format = 1095911234;
    header.color_space_id = 1;

    header.metadata.layout = _layout;
    strncpy(header.metadata.name, _fileName.c_str(), 128);

    // Create info segments

    int nslices = number_slices_from_layout(_layout);
    size_t info_slices_size = sizeof(car_rendition_info_slices) + sizeof(struct car_rendition_info_slice) * nslices;
    struct car_rendition_info_slices *info_slices = (struct car_rendition_info_slices *)malloc(info_slices_size);
    info_slices->header.magic = car_rendition_info_magic_slices;
    info_slices->header.length = info_slices_size - sizeof(car_rendition_info_header);
    // FIXME write slices
    info_slices->nslices = nslices;
    if (nslices == 1) {
        info_slices->slices[0].x = 0;
        info_slices->slices[0].y = 0;
        info_slices->slices[0].width = _width; // XXX FIXME
        info_slices->slices[0].height = _height; // XXX FIXME
    }

    struct car_rendition_info_metrics info_metrics;
    info_metrics.header.magic = car_rendition_info_magic_metrics;
    info_metrics.header.length = sizeof(struct car_rendition_info_metrics) - sizeof(struct car_rendition_info_header);
    info_metrics.nmetrics = 1;
    info_metrics.top_right_inset.width = 0;
    info_metrics.top_right_inset.height = 0;
    info_metrics.bottom_left_inset.width = 0;
    info_metrics.bottom_left_inset.height = 0;
    info_metrics.image_size.width = _width;
    info_metrics.image_size.height = _height;

    struct car_rendition_info_composition info_composition;
    info_composition.header.magic = car_rendition_info_magic_composition;
    info_composition.header.length = sizeof(struct car_rendition_info_composition) - sizeof(struct car_rendition_info_header);;
    info_composition.blend_mode = 0;
    info_composition.opacity = 1;

    struct car_rendition_info_bitmap_info info_bitmap_info;
    info_bitmap_info.header.magic = car_rendition_info_magic_bitmap_info;
    info_bitmap_info.header.length = sizeof(struct car_rendition_info_bitmap_info) - sizeof(struct car_rendition_info_header);
    info_bitmap_info.exif_orientation = 1; // XXX FIXME

    int bytes_per_pixel = 0;
    switch(this->data()->format()) {
        case Rendition::Data::Format::PremultipliedBGRA8:
            bytes_per_pixel = 4;
            break;
        case Rendition::Data::Format::PremultipliedGA8:
            bytes_per_pixel = 2;
            break;
        case Rendition::Data::Format::Data:
            break;
    }

    struct car_rendition_info_bytes_per_row info_bytes_per_row;
    info_bytes_per_row.header.magic = car_rendition_info_magic_bytes_per_row;
    info_bytes_per_row.header.length = sizeof(struct car_rendition_info_bytes_per_row) - sizeof(struct car_rendition_info_header);
    info_bytes_per_row.bytes_per_row = _width * bytes_per_pixel;

    // Write bitmap data
    ext::optional<std::vector<uint8_t>> data = _encode(this);
    if (!data) {
        printf("Error: no bitmap data\n");
        data = ext::optional<std::vector<uint8_t>>(std::vector<uint8_t>());
    }

    size_t compressed_data_length = data->size();
    uint8_t *compressed_data = &data->data()[0];

    // Assemble Header and info segments
    size_t total_header_size = sizeof(struct car_rendition_value) + info_slices_size + \
         sizeof(struct car_rendition_info_header) + info_metrics.header.length + \
         sizeof(struct car_rendition_info_header) + info_composition.header.length + \
         sizeof(struct car_rendition_info_header) + info_bitmap_info.header.length + \
         sizeof(struct car_rendition_info_header) + info_bytes_per_row.header.length;

    std::vector<uint8_t> output = std::vector<uint8_t>(total_header_size + compressed_data_length);
    uint8_t *output_bytes = &output[0];

    header.info_len = total_header_size - sizeof(struct car_rendition_value);
    header.bitmaps.bitmap_count = 1;
    header.bitmaps.payload_size = compressed_data_length;

    memcpy(output_bytes, &header, sizeof(struct car_rendition_value));
    output_bytes += sizeof(struct car_rendition_value);

    memcpy(output_bytes, info_slices, info_slices_size);
    output_bytes += info_slices_size;

    memcpy(output_bytes, &info_metrics, sizeof(struct car_rendition_info_metrics));
    output_bytes += sizeof(struct car_rendition_info_metrics);

    memcpy(output_bytes, &info_composition, sizeof(struct car_rendition_info_composition));
    output_bytes += sizeof(struct car_rendition_info_composition);

    memcpy(output_bytes, &info_bitmap_info, sizeof(struct car_rendition_info_bitmap_info));
    output_bytes += sizeof(struct car_rendition_info_bitmap_info);

    memcpy(output_bytes, &info_bytes_per_row, sizeof(struct car_rendition_info_bytes_per_row));
    output_bytes += sizeof(struct car_rendition_info_bytes_per_row);

    memcpy(output_bytes, compressed_data, compressed_data_length);

    return output;
}

