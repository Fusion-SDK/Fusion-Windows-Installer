
#pragma once

#include <string>
#include <vector>
#include "mod_registry.hpp"

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
        void BuildLoadPlan(const ModRegistry& registry);

        const std::vector<ManagedMod>& GetManagedMods() const;

        void Clear();

    private:
        std::vector<ManagedMod> managedMods_;
    };

}
