#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace snotes::ui
{

/// A link shown in the info dialog.
struct Link
{
    std::string text;
    std::string url;
};

/// A part of the program that comes from someone else, with what its license asks to be shown (Q-12).
struct ThirdPartyComponent
{
    std::string name;
    std::string author;
    std::string usage;
    std::string license;
    Link website;
    Link licenseText;
    std::string notice; ///< attribution the license or its owner asks for, empty if none
};

/// Content of the info dialog (menu "Hilfe", Pflichtenheft 6.2): the program, its own license and source
/// code (AGPLv3 section 13), how it was made and the third-party components with their licenses (Q-12).
struct AboutInfo
{
    std::string title;
    std::vector<std::string> lines;
    std::vector<Link> links;
    std::vector<ThirdPartyComponent> components;
};

/// withAsio: the Windows build with the Steinberg ASIO SDK.
[[nodiscard]] AboutInfo makeAboutInfo(std::string_view version, bool withAsio);

} // namespace snotes::ui
