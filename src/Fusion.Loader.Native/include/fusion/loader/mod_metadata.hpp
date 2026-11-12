
#pragma once

#include <string>
#include "version.hpp"

namespace fusion::loader
{
    struct ModMetadata
    {
        std::string id;
        std::string name;
        std::string author;
        Version version;

        std::uint32_t requiredLoaderApi = 1;
    };

    inline bool IsValidMetadata(const ModMetadata& metadata)
    {
        return !metadata.id.empty()
            && !metadata.name.empty()
            && !metadata.author.empty()
            && metadata.requiredLoaderApi > 0;
    }
}
