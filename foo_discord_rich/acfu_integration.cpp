#include "stdafx.h"

#include <utils/string_helpers.h>
#include <utils/version_helpers.h>
#include <utils/acfu_github.h>

#include <acfu-sdk/utils/common.h>

namespace drp::acfu
{

class SmpSource
    : public ::acfu::source
    , public utils::acfu::github_conf
{
public:
    static std::string FetchVersion()
        componentversion::ptr cv;
        service_enum_t<componentversion> e;
        for ( service_ptr_t<componentversion> ptr; e.next( ptr ); )
        {
            pfc::string8 file_name;
            ptr->get_file_name( file_name );
            if ( file_name.equals( componentFileName_ ) )
            {
                cv = ptr;
            }
        }
        if ( cv.is_empty() )
        {
            return "0.0.0";
        }

        pfc::string8 version;
        cv->get_component_version( version );
        return std::string( version.c_str(), version.length() );
    }
    GUID get_guid() override
    {
        return g_guid_drp_acfu_source;
    }
    void get_info( file_info& info ) override
    {
        if ( !isVersionFetched_ )
        {
            installedVersion_ = FetchVersion();
            isVersionFetched_ = true;
        }

        info.meta_set( "version", installedVersion_.c_str() );
        info.meta_set( "name", DRP_NAME );
        info.meta_set( "module", componentFileName_ );
    }
    bool is_newer( const file_info& info ) override
    {
        if ( !info.meta_get( "version", 0 ) || installedVersion_.empty() )
        {
            return false;
        }

        std::string available = info.meta_get( "version", 0 );
        available = utils::string::Trim( available );
        if ( available[0] == 'v' )
        {
            available.assign( available.c_str() + 1, available.length() - 1 );
        }

        return utils::version::IsNewerSemver( available, installedVersion_ );
    }
    ::acfu::request::ptr create_request() override
    {
        return fb2k::service_new<utils::acfu::github_latest_release<SmpSource>>();
    }
    static pfc::string8 get_owner()
    {
        return "TheQwertiest";
    }
    static pfc::string8 get_repo()
    {
        return componentFileName_;
    }

private:
    static constexpr char componentFileName_[] = "foo_discord_rich";
    bool isVersionFetched_ = false;
    std::string installedVersion_;
};

static service_factory_single_t<SmpSource> g_drpSource;

} // namespace drp::acfu
