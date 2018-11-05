#pragma once

#include <string>

namespace utils::version
{

/// @brief Performs comparison of semver strings
/// @return true, if string 'a' represents version newer than 'b'
bool IsNewerSemver( const std::string& a, const std::string& b );

}
