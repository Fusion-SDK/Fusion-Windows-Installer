
#include "fusion/loader/mod_loader.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
                    end == std::string::npos
                    ? value.size() - start
                    : end - start;

                if (length == 0)
                    return false;

                const char* first = value.data() + start;
                const char* last = first + length;

                const auto result = std::from_chars(
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

        const auto& discoveredMods = registry.GetMods();

        managedMods_.reserve(discoveredMods.size());

        std::unordered_map<std::string, std::vector<std::size_t>> ids;

        for (std::size_t i = 0; i < discoveredMods.size(); ++i) {
            managedMods_.push_back(ManagedMod{
                discoveredMods[i],
                ModState::Discovered,
                {}
            });

            if (discoveredMods[i].valid) {
                ids[discoveredMods[i].metadata.id].push_back(i);
            }
        }

        // Reject every occurrence of a duplicated ID.
        for (const auto& [id, indices] : ids) {
            if (indices.size() > 1) {
                for (const auto index : indices) {
                    managedMods_[index].state = ModState::Failed;
                    managedMods_[index].statusMessage =
                        "Duplicate mod ID: " + id;
                }
            }
        }

        // Validate metadata and dependencies.
        std::vector<std::vector<std::size_t>> dependents(
            discoveredMods.size()
        );

        std::vector<std::size_t> indegree(
            discoveredMods.size(), 0
        );

        for (std::size_t i = 0; i < managedMods_.size(); ++i) {
            auto& managed = managedMods_[i];

            if (!managed.discovered.valid) {
                managed.state = ModState::Failed;
                managed.statusMessage =
                    managed.discovered.error;
                continue;
            }

            if (managed.state == ModState::Failed)
                continue;

            if (managed.discovered.metadata.requiredLoaderApi >
                LoaderApiVersion) {
                managed.state = ModState::Failed;
                managed.statusMessage =
                    "A newer Fusion Loader API is required.";
                continue;
            }

            bool valid = true;

            for (const auto& dependency :
                 managed.discovered.metadata.dependencies) {

                const auto found = ids.find(dependency.id);

                if (found == ids.end()) {
                    valid = false;
                    managed.statusMessage =
                        "Missing dependency: " + dependency.id;
                    break;
                }

                if (found->second.size() != 1) {
                    valid = false;
                    managed.statusMessage =
                        "Dependency has a duplicate ID: " +
                        dependency.id;
                    break;
                }

                const std::size_t dependencyIndex =
                    found->second.front();

                const auto& dependencyMetadata =
                    managedMods_[dependencyIndex].discovered.metadata;

                if (!dependency.minimumVersion.empty() &&
                    !MeetsMinimumVersion(
                        dependencyMetadata.version,
                        dependency.minimumVersion)) {
                    valid = false;
                    managed.statusMessage =
                        "Dependency version requirement not met: " +
                        dependency.id;
                    break;
                }

                dependents[dependencyIndex].push_back(i);
                ++indegree[i];
            }

            if (valid) {
                managed.state = ModState::Validated;
                managed.statusMessage =
                    "Dependencies validated.";
            }
            else {
                managed.state = ModState::Failed;
            }
        }

        // Exclude invalid mods from the load graph.
        for (std::size_t i = 0; i < managedMods_.size(); ++i) {
            if (managedMods_[i].state == ModState::Failed) {
                for (const auto dependent : dependents[i]) {
                    if (indegree[dependent] > 0)
                        --indegree[dependent];
                }
            }
        }

        // Kahn's topological sorting algorithm.
        std::vector<std::size_t> ready;

        for (std::size_t i = 0; i < managedMods_.size(); ++i) {
            if (managedMods_[i].state == ModState::Validated &&
                indegree[i] == 0) {
                ready.push_back(i);
            }
        }

        while (!ready.empty()) {
            const std::size_t current = ready.front();
            ready.erase(ready.begin());

            loadOrder_.push_back(
                managedMods_[current].discovered.metadata.id
            );

            for (const auto dependent : dependents[current]) {
                if (managedMods_[dependent].state !=
                    ModState::Validated)
                    continue;

                if (--indegree[dependent] == 0) {
                    ready.push_back(dependent);
                }
            }
        }

        // Anything still validated but not sorted is blocked
        // by a dependency cycle.
        for (auto& managed : managedMods_) {
            if (managed.state == ModState::Validated) {
                managed.state = ModState::Failed;
                managed.statusMessage =
                    "Dependency cycle detected or mod blocked by a cycle.";
            }
        }
    }

    const std::vector<ManagedMod>&
    ModLoader::GetManagedMods() const
    {
        return managedMods_;
    }

    const std::vector<std::string>&
    ModLoader::GetLoadOrder() const
    {
        return loadOrder_;
    }

    void ModLoader::Clear()
    {
        managedMods_.clear();
        loadOrder_.clear();
    }

}
