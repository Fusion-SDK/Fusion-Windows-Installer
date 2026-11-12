
#pragma once

#include <filesystem>
#include <string>

#include "mod_metadata.hpp"

namespace fusion::loader {

    bool ParseModManifest(
        const std::filesystem::path& manifestPath,
        ModMetadata& metadata,
        std::string& error
    );

}
