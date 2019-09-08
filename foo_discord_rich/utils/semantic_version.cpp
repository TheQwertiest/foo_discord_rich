#include <stdafx.h>
#include "semantic_version.h"

#include <utils/string_helpers.h>

#include <charconv>

namespace
{

std::string_view ExtractSuffixData( std::string_view& strView, char separator )
{
    std::string_view data;
    if ( size_t pos = strView.find_first_of( separator );
         std::string::npos != pos )
    {
        data = strView.substr( pos + 1, strView.size() - pos );
        strView.remove_suffix( strView.size() - pos );
    }
    return data;
}

} // namespace

namespace drp::version
{

/// @throw std::runtime_error
SemVer::SemVer( std::string_view strVer )
{
    const auto ret = ParseString( strVer );
    if ( !ret )
    {
        throw std::runtime_error( "Parsing failed" );
    }
    *this = ret.value();
}

std::optional<SemVer> SemVer::ParseString( std::string_view strVer )
{
    SemVer semVer;

    std::string_view curScope( strVer );

    semVer.metadata = ExtractSuffixData( curScope, '+' );
    semVer.prerelease = ExtractSuffixData( curScope, '-' );

    if ( curScope.empty() )
    {
        return std::nullopt;
    }

    std::vector<std::optional<uint8_t>> versionNums;
    for ( const auto& splitView: drp::string::Split( curScope, '.' ) )
    {
        const auto numRet = drp::string::GetNumber<int8_t>( splitView );
        if ( !numRet )
        {
            break;
        }

        versionNums.push_back( numRet );
    }

    if ( versionNums.empty() || versionNums.size() > 3 )
    {
        return std::nullopt;
    }
    versionNums.resize( 3 );

    semVer.major = versionNums[0].value();
    semVer.minor = versionNums[1].value_or( 0 );
    semVer.patch = versionNums[2].value_or( 0 );

    return semVer;
}

bool SemVer::operator==( const SemVer& other ) const
{ // metadata is ignored during comparison
    return ( major == other.major
             && minor == other.minor
             && patch == other.patch
             && prerelease == other.prerelease );
}
bool SemVer::operator!=( const SemVer& other ) const
{
    return ( !( *this == other ) );
}
bool SemVer::operator<( const SemVer& other ) const
{
    if ( major != other.major )
    {
        return ( major < other.major );
    }
    if ( minor != other.minor )
    {
        return ( minor < other.minor );
    }
    if ( patch != other.patch )
    {
        return ( patch < other.patch );
    }

    // metadata is ignored during comparison
    return IsPreleaseNewer( other.prerelease, prerelease );
}
bool SemVer::operator>( const SemVer& other ) const
{
    return ( other < *this );
}
bool SemVer::operator<=( const SemVer& other ) const
{
    return ( !( other < *this ) );
}
bool SemVer::operator>=( const SemVer& other ) const
{
    return ( !( *this < other ) );
}

bool SemVer::IsPreleaseNewer( std::string_view a, std::string_view b )
{
    if ( a == b )
    {
        return false;
    }

    if ( a.empty() || b.empty() )
    { // Pre-release versions have a lower precedence than the associated normal version
        return a.empty();
    }

    const auto isNumber = []( std::string_view str ) {
        return ( str.cend() == ranges::find_if_not( str, []( char c ) { return std::isdigit( c ); } ) );
    };

    while ( !a.empty() && !b.empty() )
    {
        const std::string_view a_Token = ExtractSuffixData( a, '.' );
        const std::string_view b_Token = ExtractSuffixData( b, '.' );

        if ( a_Token != b_Token )
        {
            const bool a_isNumber = isNumber( a_Token );
            const bool b_isNumber = isNumber( b_Token );
            if ( a_isNumber != b_isNumber )
            { // Numeric identifiers always have lower precedence than non-numeric identifiers
                return !a_isNumber;
            }
            else if ( a_isNumber && b_isNumber )
            {
                auto numRet = drp::string::GetNumber<int8_t>( a_Token );
                assert( numRet ); // should be valid, because of `isNumber` check
                const int8_t aNum = numRet.value();

                numRet = drp::string::GetNumber<int8_t>( a_Token );
                assert( numRet ); // should be valid, because of `isNumber` check
                const int8_t bNum = numRet.value();

                assert( aNum != bNum ); // tokens would've been equal otherwise
                return aNum > bNum;
            }
            else
            {
                return a_Token > b_Token;
            }
        }
        else
        {
            if ( a.empty() != b.empty() )
            { // A larger set of pre-release fields has a higher precedence than a smaller set
                return !a.empty();
            }
        }
    }

    // They are equal (should not reach here)
    assert( 0 );
    return false;
}

} // namespace drp::version
