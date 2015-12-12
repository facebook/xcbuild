/**
 Copyright (c) 2015-present, Facebook, Inc.
 All rights reserved.

 This source code is licensed under the BSD-style license found in the
 LICENSE file in the root directory of this source tree. An additional grant
 of patent rights can be found in the PATENTS file in the same directory.
 */

#include <builtin/copyPlist/Driver.h>
#include <builtin/copyPlist/Options.h>
#include <plist/Object.h>
#include <plist/Format/Any.h>

using builtin::copyPlist::Driver;
using builtin::copyPlist::Options;
using libutil::FSUtil;

Driver::
Driver()
{
}

Driver::
~Driver()
{
}

std::string Driver::
name()
{
    return "builtin-copyPlist";
}

int Driver::
run(std::vector<std::string> const &args)
{
    Options options;
    std::pair<bool, std::string> result = libutil::Options::Parse<Options>(&options, args);
    if (!result.first) {
        fprintf(stderr, "error: %s\n", result.second.c_str());
        return 1;
    }

    /*
     * It's unclear if an output directory should be required, but require it for
     * now since the behavior without one is also unclear.
     */
    if (options.outputDirectory().empty()) {
        fprintf(stderr, "error: output directory not provided\n");
        return 1;
    }

    /*
     * Determine the output format. Leave null for the same as input.
     */
    std::unique_ptr<plist::Format::Any> convertFormat = nullptr;
    if (!options.convertFormat().empty()) {
        if (options.convertFormat() == "binary1") {
            convertFormat = std::unique_ptr<plist::Format::Any>(new plist::Format::Any(plist::Format::Any::Create(
                plist::Format::Binary::Create()
            )));
        } else if (options.convertFormat() == "xml1") {
            convertFormat = std::unique_ptr<plist::Format::Any>(new plist::Format::Any(plist::Format::Any::Create(
                plist::Format::XML::Create(plist::Format::Encoding::UTF8)
            )));
        } else if (options.convertFormat() == "ascii" || options.convertFormat() == "openstep1") {
            convertFormat = std::unique_ptr<plist::Format::Any>(new plist::Format::Any(plist::Format::Any::Create(
                plist::Format::ASCII::Create(plist::Format::Encoding::UTF8)
            )));
        } else {
            fprintf(stderr, "error: unknown output format %s\n", options.convertFormat().c_str());
            return 1;
        }
    }

    /*
     * Process each input.
     */
    for (std::string const &inputPath : options.inputs()) {
        /* Read in the input. */
        std::ifstream inputFile = std::ifstream(inputPath, std::ios::binary);
        if (inputFile.fail()) {
            fprintf(stderr, "error: unable to read input %s\n", inputPath.c_str());
            return 1;
        }

        std::vector<uint8_t> inputContents = std::vector<uint8_t>(std::istreambuf_iterator<char>(inputFile), std::istreambuf_iterator<char>());
        std::vector<uint8_t> outputContents;

        if (convertFormat == nullptr && !options.validate()) {
            /*
             * If we aren't converting or validating, don't even bother parsing as a plist.
             */
            outputContents = inputContents;
        } else {
            /* Determine the input format. */
            std::unique_ptr<plist::Format::Any> inputFormat = plist::Format::Any::Identify(inputContents);
            if (inputFormat == nullptr) {
                fprintf(stderr, "error: input %s is not a plist\n", inputPath.c_str());
                return 1;
            }

            /* Deserialize the input. */
            auto deserialize = plist::Format::Any::Deserialize(inputContents, *inputFormat);
            if (!deserialize.first) {
                fprintf(stderr, "error: %s: %s\n", inputPath.c_str(), deserialize.second.c_str());
                return 1;
            }

            /* Use the conversion format if specified, otherwise use the same as the input. */
            plist::Format::Any outputFormat = (convertFormat != nullptr ? *convertFormat : *inputFormat);

            /* Serialize the output. */
            auto serialize = plist::Format::Any::Serialize(deserialize.first.get(), outputFormat);
            if (serialize.first == nullptr) {
                fprintf(stderr, "error: %s: %s\n", inputPath.c_str(), serialize.second.c_str());
                return 1;
            }

            outputContents = *serialize.first;
        }

        /* Output to the same name as the input, but in the output directory. */
        std::string outputPath = options.outputDirectory() + "/" + FSUtil::GetBaseName(inputPath);

        /* Write out the output. */
        std::ofstream outputFile = std::ofstream(outputPath, std::ios::binary);
        if (outputFile.fail()) {
            fprintf(stderr, "error: could not open output path %s to write\n", outputPath.c_str());
            return 1;
        }

        std::copy(outputContents.begin(), outputContents.end(), std::ostream_iterator<char>(outputFile));
    }

    return 0;
}
