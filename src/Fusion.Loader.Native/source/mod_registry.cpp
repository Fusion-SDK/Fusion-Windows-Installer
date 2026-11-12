
#include "fusion/loader/mod_registry.hpp"

#include <fstream>
#include <sstream>

namespace fusion::loader {

    bool ModRegistry::Discover(
        const std::filesystem::path& modsDirectory)
    {
        Clear();

        if (!std::filesystem::exists(modsDirectory))
            return true;

        if (!std::filesystem::is_directory(modsDirectory))
            return false;

        for (const auto& entry :
             std::filesystem::directory_iterator(modsDirectory))
        {
            if (!entry.is_directory())
                continue;

            DiscoveredMod mod;
            mod.directory = entry.path();

            const auto manifest =
                mod.directory / "mod.json";

            if (!std::filesystem::exists(manifest))
            {
                mod.error = "Missing mod.json";
                mods_.push_back(std::move(mod));
                continue;
            }

            std::ifstream file(manifest);

            if (!file)
            {
                mod.error = "Unable to read mod.json";
                mods_.push_back(std::move(mod));
                continue;
            }

            // JSON parsing will be added with the
            // dedicated manifest parser.
            mod.error = "Manifest parser not implemented";
            mods_.push_back(std::move(mod));
        }

        return true;
    }

    const std::vector<DiscoveredMod>&
    ModRegistry::GetMods() const
    {
        return mods_;
    }

    void ModRegistry::Clear()
    {
        mods_.clear();
    }

}
