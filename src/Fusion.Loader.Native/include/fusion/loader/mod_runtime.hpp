#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "mod_loader.hpp"
#include "mod_registry.hpp"

namespace fusion::loader {

    enum class RuntimeModState {
        Loaded,
        Disabled,
        Failed
    };

    struct RuntimeModResult {
        std::string id;
        RuntimeModState state = RuntimeModState::Failed;
        std::string statusMessage;
    };

    // Loads native modules explicitly listed by validated mod manifests.
    // Native mods execute with the user's permissions; only load trusted code.
    class ModRuntime {
    public:
        ModRuntime() = default;
        ~ModRuntime();

        ModRuntime(const ModRuntime&) = delete;
        ModRuntime& operator=(const ModRuntime&) = delete;

        bool LoadAll(
            const ModLoader& loadPlan,
            const ModRegistry& registry);

        void Shutdown();

        bool IsRunning() const;
        const std::vector<RuntimeModResult>& GetResults() const;

    private:
        struct LoadedModule {
            std::string id;
            void* handle = nullptr;
            void (*unload)() = nullptr;
        };

        std::vector<LoadedModule> loaded_;
        std::vector<RuntimeModResult> results_;
        bool running_ = false;
    };

}
