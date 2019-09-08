#pragma once

#include <string>
#include <optional>

namespace drp::version
{

/// @brief See https://semver.org/
class SemVer
{
public:
    SemVer() = default;
    /// @throw std::runtime_error if parsing failed
    SemVer( std::string_view strVer );

    static std::optional<SemVer> ParseString( std::string_view strVer );

    bool operator==( const SemVer& other ) const;
    bool operator!=( const SemVer& other ) const;
    bool operator<( const SemVer& other ) const;
    bool operator>( const SemVer& other ) const;
    bool operator<=( const SemVer& other ) const;
    bool operator>=( const SemVer& other ) const;

private:
    static bool IsPreleaseNewer( std::string_view a, std::string_view b );

public:
    uint8_t major = 0;
    uint8_t minor = 0;
    uint8_t patch = 0;
    std::string prerelease;
    std::string metadata;
};

} // namespace drp::version
