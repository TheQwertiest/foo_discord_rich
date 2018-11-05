#include <nlohmann/json.hpp>
#include <acfu-sdk/acfu.h>

namespace utils::acfu
{

// Ripped from acfu-sdk/utils/github.h:
// replaced rapidjson with nlohmann::json, because don't want to have
// multiple json implementations and additional submodule dependency

using n_json = nlohmann::json;

#define ACFU_EXPECT_JSON( x ) \
    if ( !( x ) )             \
    throw std::logic_error( "unexpected JSON schema" )

struct github_conf
{
    static const char* get_owner()
    {
        throw pfc::exception_not_implemented( "implement get_owner() in derived class" );
    }

    static const char* get_repo()
    {
        throw pfc::exception_not_implemented( "implement get_repo() in derived class" );
    }

    static http_request::ptr create_http_request()
    {
        return static_api_ptr_t<http_client>()->create_request( "GET" );
    }

    static bool is_acceptable_release( const n_json& release )
    {
        return true;
    }

    static bool is_acceptable_asset( const n_json& asset )
    {
        return false;
    }
};

template <class t_github_conf>
class github_releases
    : public ::acfu::request
{
public:
    virtual void run( file_info& info, abort_callback& abort )
    {
        pfc::string8 url = form_releases_url();
        http_request::ptr request = t_github_conf::create_http_request();

        file::ptr response = request->run_ex( url.get_ptr(), abort );
        pfc::array_t<uint8_t> data;
        response->read_till_eof( data, abort );

        http_reply::ptr reply;
        if ( !response->cast( reply ) )
        {
            throw exception_service_extension_not_found();
        }

        n_json doc;
        try
        {
            doc = n_json::parse( std::string{ (const char*)data.get_ptr(), data.get_count() } );
        }
        catch ( const n_json::parse_error& err )
        {
            throw exception_io_data( PFC_string_formatter()
                                     << "error: " << err.what() );
        }

        pfc::string8 status;
        reply->get_status( status );
        // RFC: Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
        auto pos = status.find_first( ' ' );
        if ( ~0 == pos || 0 != pfc::strcmp_partial( status.get_ptr() + pos + 1, "200 " ) )
        {
            process_error( doc, status.get_ptr() );
        }
        else
        {
            process_response( doc, info );
        }
    }

protected:
    virtual pfc::string8 form_releases_url()
    {
        pfc::string8 url;
        url << "https://api.github.com/repos/" << t_github_conf::get_owner()
            << "/" << t_github_conf::get_repo() << "/releases";
        return url;
    }

    virtual void process_asset( const n_json& asset, file_info& info )
    {
        const auto assetRaw = asset.dump();
        info.info_set_ex( "asset", pfc_infinite, assetRaw.c_str(), assetRaw.length() );

        if ( auto it = asset.find( "browser_download_url" ); it != asset.end() && it->is_string() )
        {
            info.meta_set( "download_url", it.value().get<std::string>().c_str() );
        }
    }

    virtual void process_release( const n_json& release, file_info& info )
    {
        {
            auto it = release.find( "tag_name" );
            ACFU_EXPECT_JSON( it != release.end() && it->is_string() );
            info.meta_set( "version", it.value().get<std::string>().c_str() );
        }
        {
            auto it = release.find( "html_url" );
            ACFU_EXPECT_JSON( it != release.end() && it->is_string() );
            info.meta_set( "download_page", it.value().get<std::string>().c_str() );
        }

        if ( auto it = release.find( "assets" ); it != release.end() && it->is_array() )
        {
            auto assets = it.value();
            for ( const auto& asset : assets )
            {
                if ( t_github_conf::is_acceptable_asset( asset ) )
                {
                    process_asset( asset, info );
                    break;
                }
            }
        }

        const auto releaseRaw = release.dump();
        info.info_set_ex( "release", pfc_infinite, releaseRaw.c_str(), releaseRaw.length() );
    }

    virtual void process_response( const n_json& json, file_info& info )
    {
        ACFU_EXPECT_JSON( json.is_array() );
        for ( const auto& release : json )
        {
            if ( t_github_conf::is_acceptable_release( release ) )
            {
                return process_release( release, info );
            }
        }
    }

private:
    void process_error( const n_json& json, const char* http_status )
    {
        if ( auto it = json.find( "message" ); it != json.end() && it->is_string() )
        {
            throw exception_io_data( it.value().get<std::string>().c_str() );
        }
        else
        {
            throw exception_io_data( PFC_string_formatter()
                                     << "unexpected response; HTTP status: " << http_status );
        }
    }
};

template <class t_github_conf>
class github_latest_release
    : public github_releases<t_github_conf>
{
protected:
    virtual pfc::string8 form_releases_url()
    {
        pfc::string8 url;
        url << "https://api.github.com/repos/" << t_github_conf::get_owner()
            << "/" << t_github_conf::get_repo() << "/releases/latest";
        return url;
    }

    virtual void process_response( const n_json& json, file_info& info )
    {
        ACFU_EXPECT_JSON( json.is_object() );
        if ( t_github_conf::is_acceptable_release( json ) )
        {
            process_release( json, info );
        }
    }
};

} // namespace utils::acfu
