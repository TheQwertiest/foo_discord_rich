#include <stdafx.h>
#include "version_helpers.h"

#include <optional>
#include <cctype>

// TODO: replace stol with from_chars

namespace
{

struct SemVer
{ // see https://semver.org/
    bool operator>( SemVer& other )
    {
        if ( major != other.major )
        {
            return ( major > other.major );
        }
        if ( minor != other.minor )
        {
            return ( minor > other.minor );
        }
        if ( patch != other.patch )
        {
            return ( patch > other.patch );
        }

        // metadata is ignored during comparison
        return IsPreleaseNewer( prerelease, other.prerelease );
    }

    uint8_t major = 0;
    uint8_t minor = 0;
    uint8_t patch = 0;
    std::string_view prerelease;
    std::string_view metadata;

private:
    static bool IsPreleaseNewer( std::string_view a, std::string_view b )
    {
        if ( a == b )
        {
            return false;
        }

        if ( a.empty() || b.empty() )
        { // Pre-release versions have a lower precedence than the associated normal version
            return a.empty();
        }

        auto isNumber = []( std::string_view str ) {
            return ( str.cend() == std::find_if_not( str.cbegin(), str.cend(), []( char c ) { return std::isdigit( c ); } ) );
        };

        while ( !a.empty() && !b.empty() )
        {
            std::string_view aStr( a );
            std::string_view bStr( b );

            if ( size_t pos = aStr.find_first_of( '.' ); std::string::npos != pos )
            {
                aStr = aStr.substr( 0, pos );
                a.remove_prefix( pos + 1 );
            }
            else
            {
                a = std::string_view{};
            }
            if ( size_t pos = bStr.find_first_of( '.' ); std::string::npos != pos )
            {
                bStr = bStr.substr( 0, pos );
                a.remove_prefix( pos + 1 );
            }
            else
            {
                b = std::string_view{};
            }

            bool a_isNumber = isNumber( aStr );
            bool b_isNumber = isNumber( bStr );
            if ( a_isNumber != b_isNumber )
            { // Numeric identifiers always have lower precedence than non-numeric identifiers
                return !a_isNumber;
            }

            if ( a_isNumber && b_isNumber )
            {
                std::string numberStr( aStr.data(), aStr.size() );
                size_t dummy = 0; // should be valid
                int8_t aNum = std::stoi( numberStr, &dummy );
                assert( dummy );

                numberStr.assign( bStr.data(), bStr.size() );
                int8_t bNum = std::stoi( numberStr, &dummy );
                assert( dummy );

                if ( aNum != bNum )
                {
                    return aNum > bNum;
                }
            }
            if ( aStr != bStr )
            {
                return aStr > bStr;
            }

            if ( a.empty() != b.empty() )
            { // A larger set of pre-release fields has a higher precedence than a smaller set
                return !a.empty();
            }
        }

        // They are equal
        return false;
    }
};

bool ParseString( const std::string& strVer, SemVer& semVer )
{
    std::string_view curScope( strVer );
    if ( size_t pos = curScope.find_first_of( '+' ); std::string::npos != pos )
    {
        semVer.metadata = curScope.substr( pos + 1, curScope.size() - pos );
        curScope.remove_suffix( curScope.size() - pos );
    }

    if ( size_t pos = curScope.find_first_of( '-' ); std::string::npos != pos )
    {
        semVer.prerelease = curScope.substr( pos + 1, curScope.size() - pos );
        curScope.remove_suffix( curScope.size() - pos );
    }
    if ( !curScope.size() )
    {
        return false;
    }

    auto getNumber = []( std::string_view& numberStrView ) -> std::optional<uint8_t> {
        try
        {
            const std::string numberStr( numberStrView.data(), numberStrView.size() );

            size_t idx = 0;
            int8_t number = std::stoi( numberStr, &idx );
            if ( !idx )
            {
                return std::nullopt;
            }

            numberStrView.remove_prefix( idx );
            return number;
        }
        catch ( const std::invalid_argument& )
        {
            return std::nullopt;
        }
        catch ( const std::out_of_range& )
        {
            return std::nullopt;
        }
    };

    {
        auto majorNumber = getNumber( curScope );
        if ( !majorNumber || ( !curScope.empty() && curScope[0] != '.' ) )
        {
            return false;
        }

        semVer.major = majorNumber.value();
        if ( curScope.empty() )
        {
            return true;
        }
    }

    {
        curScope.remove_prefix( 1 );
        auto minorNumber = getNumber( curScope );
        if ( !minorNumber || ( !curScope.empty() && curScope[0] != '.' ) )
        {
            return false;
        }

        semVer.minor = minorNumber.value();
        if ( curScope.empty() )
        {
            return true;
        }
    }

    {
        curScope.remove_prefix( 1 );
        auto patchNumber = getNumber( curScope );
        if ( !patchNumber || !curScope.empty() )
        {
            return false;
        }

        semVer.patch = patchNumber.value();
    }

    return true;
};

} // namespace

namespace utils::version
{

bool IsNewerSemver( const std::string& a, const std::string& b )
{
    SemVer a_semVer;
    SemVer b_semVer;
    if ( !ParseString( a, a_semVer )
         || !ParseString( b, b_semVer ) )
    {
        assert( 0 );
        return false;
    }

    return ( a_semVer > b_semVer );
}

} // namespace utils::version
