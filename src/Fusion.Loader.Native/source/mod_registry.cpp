
#include "fusion/loader/mod_registry.hpp"
#include "fusion/loader/manifest_parser.hpp"

namespace fusion::loader {

    bool ModRegistry::Discover(
        const std::filesystem::path& modsDirectory)
    {
        Clear();

        std::error_code ec;

        if (!std::filesystem::exists(modsDirectory, ec)) {
            return !ec;
        }

        if (ec || !std::filesystem::is_directory(modsDirectory, ec)) {
            return false;
        }

        for (const auto& entry :
             std::filesystem::directory_iterator(modsDirectory, ec))
        {
            if (ec)
                return false;

            if (!entry.is_directory(ec) || ec)
                continue;

            DiscoveredMod mod;
            mod.directory = entry.path();

            const auto manifest = mod.directory / "mod.json";

            if (!std::filesystem::exists(manifest, ec) || ec) {
                mod.error = "Missing mod.json.";
                mods_.push_back(std::move(mod));
                continue;
            }

            mod.valid = ParseModManifest(
                manifest,
                mod.metadata,
                mod.error
            );

            mods_.push_back(std::move(mod));
        }

        return !ec;
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
