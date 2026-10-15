#include "ui/AboutInfo.h"

#include "i18n/Translate.h"

#include <utility>

namespace snotes::ui
{
namespace
{

ThirdPartyComponent juce()
{
    return {.name = "JUCE 8",
            .author = "Raw Material Software Limited",
            .usage = i18n::tr("User interface, audio drivers, audio files"),
            .license = "GNU Affero General Public License v3.0",
            .website = {"juce.com", "https://juce.com"},
            .licenseText = {"AGPLv3", "https://www.gnu.org/licenses/agpl-3.0.html"},
            .notice = {}};
}

// Libraries JUCE builds into the program. They are appended one by one: a brace-initialized vector of
// these structs leaves exception branches no test can reach (coverage gate, E-02).

/// Reads FLAC files.
ThirdPartyComponent flac()
{
    return {.name = "FLAC",
            .author = "Josh Coalson, Xiph.Org Foundation",
            .usage = i18n::tr("Reading FLAC files (included in JUCE)"),
            .license = "BSD-3-Clause",
            .website = {"xiph.org/flac", "https://xiph.org/flac/"},
            .licenseText = {"BSD-3-Clause", "https://opensource.org/license/bsd-3-clause"},
            .notice = {}};
}

/// Reads Ogg Vorbis files.
ThirdPartyComponent oggVorbis()
{
    return {.name = "Ogg Vorbis",
            .author = "Xiph.Org Foundation",
            .usage = i18n::tr("Reading Ogg Vorbis files (included in JUCE)"),
            .license = "BSD-3-Clause",
            .website = {"xiph.org/vorbis", "https://xiph.org/vorbis/"},
            .licenseText = {"BSD-3-Clause", "https://opensource.org/license/bsd-3-clause"},
            .notice = {}};
}

ThirdPartyComponent zlib()
{
    return {.name = "zlib",
            .author = "Jean-loup Gailly, Mark Adler",
            .usage = i18n::tr("Compression (included in JUCE)"),
            .license = "zlib License",
            .website = {"zlib.net", "https://zlib.net"},
            .licenseText = {"zlib License", "https://zlib.net/zlib_license.html"},
            .notice = {}};
}

ThirdPartyComponent libpng()
{
    return {.name = "libpng",
            .author = "Contributing Authors, Glenn Randers-Pehrson, Cosmin Truta",
            .usage = i18n::tr("PNG images (included in JUCE)"),
            .license = "PNG Reference Library License",
            .website = {"libpng.org", "http://www.libpng.org/pub/png/libpng.html"},
            .licenseText = {"libpng License", "http://www.libpng.org/pub/png/src/libpng-LICENSE.txt"},
            .notice = {}};
}

ThirdPartyComponent ijgJpeg()
{
    return {.name = "IJG JPEG Library",
            .author = "Independent JPEG Group",
            .usage = i18n::tr("JPEG images (included in JUCE)"),
            .license = "IJG License",
            .website = {"ijg.org", "https://ijg.org"},
            .licenseText = {"IJG License", "https://ijg.org/files/README"},
            .notice = "This software is based in part on the work of the Independent JPEG Group."};
}

ThirdPartyComponent asioSdk()
{
    return {.name = "ASIO SDK 2.3.4",
            .author = "Steinberg Media Technologies GmbH",
            .usage = i18n::tr("ASIO drivers on Windows"),
            .license = "GNU General Public License v3.0",
            .website = {"steinberg.net/developers", "https://www.steinberg.net/developers/"},
            .licenseText = {"GPLv3", "https://www.gnu.org/licenses/gpl-3.0.html"},
            .notice = "ASIO is a trademark and software of Steinberg Media Technologies GmbH."};
}

/// Runs the neural network of the note detection (F-TX-02).
ThirdPartyComponent onnxRuntime()
{
    return {.name = "ONNX Runtime 1.31",
            .author = "Microsoft Corporation",
            .usage = i18n::tr("Runs the neural network of the note detection"),
            .license = "MIT License",
            .website = {"onnxruntime.ai", "https://onnxruntime.ai"},
            .licenseText = {"MIT", "https://github.com/microsoft/onnxruntime/blob/main/LICENSE"},
            .notice = "Copyright (c) Microsoft Corporation."};
}

/// Draws notes and tablature (F-NA-01). LGPLv3: linked statically, so the source code of the whole program
/// is public (AGPLv3), which lets anyone relink it with a changed Verovio.
ThirdPartyComponent verovio()
{
    return {.name = "Verovio 6.3",
            .author = "RISM Digital Center and contributors",
            .usage = i18n::tr("Draws the notes and the tablature"),
            .license = "GNU Lesser General Public License v3.0",
            .website = {"verovio.org", "https://www.verovio.org"},
            .licenseText = {"LGPLv3", "https://www.gnu.org/licenses/lgpl-3.0.html"},
            .notice = {}};
}

} // namespace

AboutInfo makeAboutInfo(std::string_view version, bool withAsio)
{
    AboutInfo info;
    info.title = "Scratch Notes " + std::string{version};
    info.lines.push_back(i18n::tr("Record improvisations and write them down as notes and tablature."));
    info.lines.push_back(
        i18n::tr("Free software under the GNU Affero General Public License v3.0, without any "
                 "warranty. The source code is public."));
    info.lines.push_back(i18n::tr("Made with Claude (Anthropic) and Claude Code."));
    info.links.push_back({i18n::tr("Source code on GitHub"), "https://github.com/ladych/ScratchNotes"});
    info.links.push_back({i18n::tr("License AGPLv3"), "https://www.gnu.org/licenses/agpl-3.0.html"});
    info.links.push_back({"Claude Code", "https://claude.com/claude-code"});
    for (auto* const make : {&juce, &flac, &oggVorbis, &zlib, &libpng, &ijgJpeg})
        info.components.push_back(make());
    if (withAsio)
        info.components.push_back(asioSdk());
    info.components.push_back(onnxRuntime());
    info.components.push_back(verovio());
    return info;
}

} // namespace snotes::ui
