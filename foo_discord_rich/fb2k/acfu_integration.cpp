#include <stdafx.h>

#include <component_guids.h>

#include <qwr/acfu_integration.h>

using namespace drp;

namespace
{

class DRPSource
    : public qwr::acfu::QwrSource
{
public:
    // ::acfu::source
    [[nodiscard]] GUID get_guid() override;
    ::acfu::request::ptr create_request() override;

    // qwr::acfu::github_conf
    [[nodiscard]] static pfc::string8 get_owner();
    [[nodiscard]] static pfc::string8 get_repo();

    // qwr::acfu::QwrSource
    [[nodiscard]] std::string GetComponentName() const override;
    [[nodiscard]] std::string GetComponentFilename() const override;
};

} // namespace

namespace
{

GUID DRPSource::get_guid()
{
    return guid::acfu_source;
}

::acfu::request::ptr DRPSource::create_request()
{
    return fb2k::service_new<qwr::acfu::github_latest_release<DRPSource>>();
}

pfc::string8 DRPSource::get_owner()
{
    return "TheQwertiest";
}

pfc::string8 DRPSource::get_repo()
{
    return DRP_UNDERSCORE_NAME;
}

std::string DRPSource::GetComponentName() const
{
    return DRP_NAME;
}

std::string DRPSource::GetComponentFilename() const
{
    return DRP_UNDERSCORE_NAME;
}

FB2K_SERVICE_FACTORY( DRPSource );

} // namespace
