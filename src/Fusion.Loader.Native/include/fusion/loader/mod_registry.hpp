
#pragma once

#include <filesystem>
#include <vector>
#include "mod_metadata.hpp"

namespace fusion::loader {

    struct DiscoveredMod {
        ModMetadata metadata;
        std::filesystem::path directory;
        bool valid = false;
        std::string error;
    };

    class ModRegistry {
    public:
        bool Discover(const std::filesystem::path& modsDirectory);

        const std::vector<DiscoveredMod>& GetMods() const;

        void Clear();

    private:
        std::vector<DiscoveredMod> mods_;
    };

}
