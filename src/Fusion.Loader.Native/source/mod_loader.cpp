
#include "fusion/loader/mod_loader.hpp"

namespace fusion::loader {

    void ModLoader::BuildLoadPlan(
        const ModRegistry& registry)
    {
        Clear();

        for (const auto& discovered : registry.GetMods())
        {
            ManagedMod managed;
            managed.discovered = discovered;

            if (!discovered.valid)
            {
                managed.state = ModState::Failed;
                managed.statusMessage =
                    discovered.error.empty()
                    ? "Mod validation failed."
                    : discovered.error;
            }
            else
            {
                managed.state = ModState::Validated;
                managed.statusMessage = "Ready for compatibility checks.";
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
