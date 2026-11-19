#include "fusion/loader/mod_registry.hpp"
#include "fusion/loader/manifest_parser.hpp"

namespace fusion::loader {

    bool ModRegistry::Discover(
        const std::filesystem::path& modsDirectory)
    {
        Clear();

        std::error_code ec;
        if (!std::filesystem::exists(modsDirectory, ec))
            return !ec;
        if (ec)
            return false;

        const auto rootStatus = std::filesystem::symlink_status(modsDirectory, ec);
        if (ec || std::filesystem::is_symlink(rootStatus) ||
            !std::filesystem::is_directory(rootStatus))
            return false;

        const auto root = std::filesystem::canonical(modsDirectory, ec);
        if (ec)
            return false;

        for (const auto& entry :
             std::filesystem::directory_iterator(root, ec))
        {
            if (ec)
                return false;

            DiscoveredMod mod;
            mod.directory = entry.path();

            const auto entryStatus = entry.symlink_status(ec);
            if (ec)
                return false;

            if (std::filesystem::is_symlink(entryStatus)) {
                mod.error = "Symbolic links are not permitted in the Fusion mods directory.";
                mods_.push_back(std::move(mod));
                continue;
            }

            if (!std::filesystem::is_directory(entryStatus))
                continue;

            const auto canonicalDirectory =
                std::filesystem::canonical(entry.path(), ec);
            if (ec || canonicalDirectory.parent_path() != root) {
                mod.error = "Mod directory does not resolve to a direct child of the Fusion mods directory.";
                mods_.push_back(std::move(mod));
                ec.clear();
                continue;
            }

            mod.directory = canonicalDirectory;
            const auto manifest = canonicalDirectory / "mod.json";
            const auto manifestStatus = std::filesystem::symlink_status(manifest, ec);

            if (ec || !std::filesystem::exists(manifestStatus)) {
                mod.error = "Missing mod.json.";
                mods_.push_back(std::move(mod));
                ec.clear();
                continue;
            }

            if (std::filesystem::is_symlink(manifestStatus) ||
                !std::filesystem::is_regular_file(manifestStatus)) {
                mod.error = "Mod manifest must be a regular file and cannot be a symbolic link.";
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
