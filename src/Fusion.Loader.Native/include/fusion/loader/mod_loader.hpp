#pragma once

#include <string>
#include <vector>

#include "mod_registry.hpp"
#include "mod_settings.hpp"

namespace fusion::loader {

    enum class ModState {
        Discovered,
        Validated,
        Disabled,
        Failed
    };

    struct ManagedMod {
        DiscoveredMod discovered;
        ModState state = ModState::Discovered;
        std::string statusMessage;
    };

    class ModLoader {
    public:
        // Without a settings store, every valid mod is enabled by default.
        void BuildLoadPlan(const ModRegistry& registry);
        void BuildLoadPlan(
            const ModRegistry& registry,
            const ModSettingsStore& settings);

        const std::vector<ManagedMod>& GetManagedMods() const;
        const std::vector<std::string>& GetLoadOrder() const;
        void Clear();

    private:
        void BuildLoadPlanInternal(
            const ModRegistry& registry,
            const ModSettingsStore* settings);

        std::vector<ManagedMod> managedMods_;
        std::vector<std::string> loadOrder_;
    };

}
