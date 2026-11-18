#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "version.hpp"

namespace fusion::loader {

    struct ModDependency {
        std::string id;
        std::string minimumVersion;
    };

    struct ModFileHash {
        std::string path;
        std::string sha256;
    };

    struct ModMetadata {
        std::string id;
        std::string name;
        std::string author;
        std::string version;
        std::string library;
        std::uint32_t requiredLoaderApi = 1;
        std::uint32_t packageFormatVersion = 1;
        std::vector<ModDependency> dependencies;
        std::vector<ModFileHash> files;
    };

    inline bool IsValidMetadata(const ModMetadata& metadata)
    {
        return !metadata.id.empty()
            && !metadata.name.empty()
            && !metadata.author.empty()
            && !metadata.version.empty()
            && metadata.requiredLoaderApi > 0;
    }

}
