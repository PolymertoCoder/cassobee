#pragma once
#include <string>

namespace bee::util
{

// url
std::string url_encode(std::string_view str, bool space_as_plus = true);
std::string url_decode(std::string_view str, bool space_as_plus = true);

} // namespace bee::util