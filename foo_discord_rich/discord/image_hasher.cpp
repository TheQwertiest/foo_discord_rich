#include <stdafx.h>

#include <nlohmann/json.hpp>

#include "image_hasher.h"

#include "component_defines.h"
#include "foobar2000/SDK/component.h"

namespace drp::uploader
{

pfc::string8 get_image_hash_file(abort_callback &abort)
{
    static const auto path = core_api::pathInProfile(DRP_UNDERSCORE_NAME);
    auto filename = pfc::string8(path);
    filename.add_filename("image_hashes.json");

    if (!filesystem::g_exists(path.c_str(), abort))
    {
        filesystem::g_create_directory(path.c_str(), abort);
    }

    return filename;
}

bool restore_old_hash_json(abort_callback &abort)
{
    const auto &filename = get_image_hash_file(abort);
    auto filename_old = filename;
    filename_old += ".old";

    if (filesystem::g_exists(filename_old.c_str(), abort))
    {
        FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Restoring old artwork hash json file because the actual file one was not found or it was corrupted";
        filesystem::g_copy(filename_old, filename, abort);
        return true;
    }

    return false;
}

/* Parses image hash json into a unique ptr */
bool read_hash_json(std::unique_ptr<nlohmann::json> &json, abort_callback &abort)
{
    const auto &filename = get_image_hash_file(abort);
    if ( !filesystem::g_exists(filename.c_str(), abort) && !restore_old_hash_json(abort) )
    {
        return false;
    }

    {
        pfc::string8 json_string;
        // Files needs to be scoped like this so it gets released immediately after reading
        {
            service_ptr_t<file> file;
            filesystem::g_open_read( file, filename.c_str(), abort );

            file->read_string_raw( json_string, abort );
        }

        try
        {
            json = std::make_unique<nlohmann::json>( nlohmann::json::parse(std::string( json_string.c_str() )));
        }
        catch (const nlohmann::detail::parse_error&)
        {
            FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Image hash json parsing failed. Trying to restore old file";
            if ( restore_old_hash_json( abort ) )
            {
                service_ptr_t<file> file;
                filesystem::g_open_read( file, filename.c_str(), abort );
                file->read_string_raw( json_string, abort );
                json = std::make_unique<nlohmann::json>( nlohmann::json::parse( std::string( json_string.c_str() ) ) );
            }
        }
    }

    return true;
}

static std::unique_ptr<nlohmann::json> g_hashJson;
bool prepare_static_hash_json( abort_callback& abort )
{
    static bool loaded = false;
    if (!loaded)
    {
        if ( read_hash_json( g_hashJson, abort ) )
        {
            loaded = true;
        }
    }

    return loaded;
}


bool check_artwork_hash(artwork_info &artwork, abort_callback &abort, pfc::string8 &artwork_url)
{
    const auto md5 = static_api_ptr_t<hasher_md5>()->process_single(artwork.data->get_ptr(), artwork.data->get_size()).asString();
    artwork.artwork_hash = md5;

    if ( !prepare_static_hash_json( abort ) )
    {
        return false;
    }

    if (g_hashJson->contains(md5.c_str()))
    {
        artwork_url = pfc::string8(
            g_hashJson->at(md5.toString()).get<std::string>().c_str()
        );
        FB2K_console_formatter() << DRP_NAME_WITH_VERSION << ": Found url " << artwork_url << " with image hash " << md5;

        return true;
    }

    return false;
}

/**
 * Saves the hash json file making sure existing data is not lost in case of an error
 */
bool save_hash_json(abort_callback &abort)
{
    if ( !prepare_static_hash_json( abort) )
    {
        return false;
    }

    const auto &filename = get_image_hash_file(abort);
    auto filename_new = pfc::string8(filename);
    filename_new += ".new";

    auto filename_old = pfc::string8(filename);
    filename_old += ".old";

    // Write current json to file
    {
        service_ptr_t<file> file_ptr;
        filesystem::g_open_write_new(file_ptr, filename_new, abort);
        file_ptr->write_string_raw(g_hashJson->dump(4).c_str(), abort);
    }

    // Copy the old file to backup file
    filesystem::g_copy(filename, filename_old, abort);

    // Copy the new file to the main file
    filesystem::g_copy(filename_new, filename, abort);

    return true;
}


bool set_artwork_url_hash(const pfc::string8 &artwork_url, const pfc::string8 &artwork_hash, abort_callback &abort)
{
    if ( !prepare_static_hash_json( abort) )
    {
        return false;
    }

    (*g_hashJson)[artwork_hash.c_str()] = artwork_url.c_str();

    // Should be fine to call this for every new entry as it's only reached from upload at the moment and that
    // has a concurrency lock
    return save_hash_json(abort);
}

class initquit_impl : public initquit
{
public:
    void on_quit()
    {
        g_hashJson.release();
    }
};

static service_factory_single_t<initquit_impl> g_initquit_impl;

} // namespace drp::uploader
