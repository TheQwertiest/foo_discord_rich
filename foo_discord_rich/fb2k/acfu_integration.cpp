#include <stdafx.h>

#include <qwr/acfu_integration.h>

using namespace drp;

namespace
{

class DrpSource
    : public qwr::acfu::QwrSource
{
public:
    // ::acfu::source
    GUID get_guid() override;
    ::acfu::request::ptr create_request() override;

    // qwr::acfu::github_conf
    static pfc::string8 get_owner();
    static pfc::string8 get_repo();

    // qwr::acfu::QwrSource
    std::string GetComponentName() const override;
    std::string GetComponentFilename() const override;
};

} // namespace

namespace
{

GUID DrpSource::get_guid()
{
    return guid::acfu_source;
}

::acfu::request::ptr DrpSource::create_request()
{
    return fb2k::service_new<qwr::acfu::github_latest_release<DrpSource>>();
}

pfc::string8 DrpSource::get_owner()
{
    return "TheQwertiest";
}

pfc::string8 DrpSource::get_repo()
{
    return "https://github.com/TheQwertiest/" DRP_UNDERSCORE_NAME;
}

std::string DrpSource::GetComponentName() const
{
    return DRP_NAME;
}

std::string DrpSource::GetComponentFilename() const
{
    return DRP_UNDERSCORE_NAME;
}

service_factory_single_t<DrpSource> g_acfuSource;

} // namespace
