
#include "fusion/loader/mod_loader.hpp"

#include <charconv>
#include <cstdint>
#include <unordered_map>

namespace fusion::loader {

    namespace {

        bool ParseVersion(
            const std::string& value,
            std::uint32_t& major,
            std::uint32_t& minor,
            std::uint32_t& patch)
        {
            std::uint32_t* parts[] = {
                &major, &minor, &patch
            };

            std::size_t start = 0;

            for (int i = 0; i < 3; ++i) {
                const std::size_t end =
                    value.find('.', start);

                if ((i < 2 && end == std::string::npos) ||
                    (i == 2 && end != std::string::npos))
                    return false;

                const std::size_t length =
                    (end == std::string::npos)
                    ? value.size() - start
                    : end - start;

                if (length == 0)
                    return false;

                const char* first = value.data() + start;
                const char* last = first + length;

                auto result = std::from_chars(
                    first, last, *parts[i]
                );

                if (result.ec != std::errc{} ||
                    result.ptr != last)
                    return false;

                if (end == std::string::npos)
                    break;

                start = end + 1;
            }

            return true;
        }

        bool MeetsMinimumVersion(
            const std::string& installed,
            const std::string& minimum)
        {
            std::uint32_t a1, a2, a3;
            std::uint32_t b1, b2, b3;

            if (!ParseVersion(installed, a1, a2, a3) ||
                !ParseVersion(minimum, b1, b2, b3))
                return false;

            if (a1 != b1) return a1 > b1;
            if (a2 != b2) return a2 > b2;

            return a3 >= b3;
        }

    }

    void ModLoader::BuildLoadPlan(
        const ModRegistry& registry)
    {
        Clear();

        std::unordered_map<std::string, const DiscoveredMod*> byId;

        for (const auto& discovered : registry.GetMods()) {
            if (!discovered.valid)
                continue;

            auto [it, inserted] = byId.emplace(
                discovered.metadata.id,
                &discovered
            );

            if (!inserted) {
                byId.erase(discovered.metadata.id);
            }
        }

        for (const auto& discovered : registry.GetMods()) {
            ManagedMod managed;
            managed.discovered = discovered;

            if (!discovered.valid) {
                managed.state = ModState::Failed;
                managed.statusMessage = discovered.error;
            }
            else if (discovered.metadata.requiredLoaderApi >
                     LoaderApiVersion) {
                managed.state = ModState::Failed;
                managed.statusMessage =
                    "This mod requires a newer Fusion Loader API.";
            }
            else {
                bool dependenciesValid = true;

                for (const auto& dependency :
                     discovered.metadata.dependencies) {

                    auto found = byId.find(dependency.id);

                    if (found == byId.end()) {
                        dependenciesValid = false;
                        managed.statusMessage =
                            "Missing or duplicate dependency: " +
                            dependency.id;
                        break;
                    }

                    if (!dependency.minimumVersion.empty() &&
                        !MeetsMinimumVersion(
                            found->second->metadata.version,
                            dependency.minimumVersion)) {

                        dependenciesValid = false;
                        managed.statusMessage =
                            "Dependency version is too old: " +
                            dependency.id;
                        break;
                    }
                }

                if (dependenciesValid) {
                    managed.state = ModState::Validated;
                    managed.statusMessage =
                        "Manifest and dependencies validated.";
                }
                else {
                    managed.state = ModState::Failed;
                }
            }

            managedMods_.push_back(std::move(managed));
        }
    }

    const std::vector<ManagedMod>&
    ModLoader::GetManagedMods() const
    {
        return managedMods_;
    }

    void ModLoader::Clear()
    {
        managedMods_.clear();
    }

}
