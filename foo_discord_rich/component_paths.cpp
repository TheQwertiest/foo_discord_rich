#include <stdafx.h>

#include "component_paths.h"

#include <qwr/fb2k_paths.h>

namespace drp::path
{

std::filesystem::path ImageDir()
{
    return qwr::path::Profile() / DRP_UNDERSCORE_NAME / "images";
}

} // namespace drp::path
