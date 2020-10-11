#include <stdafx.h>

#include <utils/string_helpers.h>
#include <utils/semantic_version.h>
#include <utils/acfu_github.h>

#include <acfu-sdk/utils/common.h>

using namespace drp::acfu;

namespace
{

class DrpSource
    : public ::acfu::source
    , public drp::acfu::github_conf
{
public:
    GUID get_guid() override;
    void get_info( file_info& info ) override;
    bool is_newer( const file_info& info ) override;
    ::acfu::request::ptr create_request() override;

    static pfc::string8 get_owner();
    static pfc::string8 get_repo();

private:
    static std::string FetchVersion();

private:
    static constexpr char componentName_[] = DRP_NAME;
    static constexpr char componentFileName_[] = DRP_UNDERSCORE_NAME;

    bool isVersionFetched_ = false;
    std::string installedVersion_;
};

} // namespace

namespace
{

GUID DrpSource::get_guid()
{
    return drp::guid::acfu_source;
}

void DrpSource::get_info( file_info& info )
{
    if ( !isVersionFetched_ )
    {
        installedVersion_ = FetchVersion();
        isVersionFetched_ = true;
    }

    info.meta_set( "version", installedVersion_.c_str() );
    info.meta_set( "name", componentName_ );
    info.meta_set( "module", componentFileName_ );
}

bool DrpSource::is_newer( const file_info& info )
{
    if ( !info.meta_get( "version", 0 ) || installedVersion_.empty() )
    {
        return false;
    }

	const std::u8string availableVersion = [&info]() {
        std::u8string version = info.meta_get( "version", 0 );
        version = drp::string::Trim<char8_t>( version );
        if ( version[0] == 'v' )
        {
            version = version.substr( 1 );
        }
        return version;
	}();

    try
    {
        return ( drp::version::SemVer{ availableVersion } > drp::version::SemVer{ installedVersion_ } );
    }
    catch ( const std::runtime_error& )
    {
        assert( false );
        return false;
    }
}

::acfu::request::ptr DrpSource::create_request()
{
    return fb2k::service_new<drp::acfu::github_latest_release<DrpSource>>();
}

pfc::string8 DrpSource::get_owner()
{
    return "TheQwertiest";
}

pfc::string8 DrpSource::get_repo()
{
    return componentFileName_;
}

std::string DrpSource::FetchVersion()
{
    auto cvRet = []() -> std::optional<componentversion::ptr> {
        for ( service_enum_t<componentversion> e; !e.finished(); ++e )
        {
            auto cv = e.get();

            pfc::string8 file_name;
            cv->get_file_name( file_name );
            if ( file_name.equals( componentFileName_ ) )
            {
                return cv;
            }
        }

        return std::nullopt;
    }();

    if ( !cvRet )
    {
        return "0.0.0";
    }
    else
    {
        pfc::string8 version;
        cvRet.value()->get_component_version( version );
        return std::string( version.c_str(), version.length() );
    }
}

service_factory_single_t<DrpSource> g_smpSource;

} // namespace
