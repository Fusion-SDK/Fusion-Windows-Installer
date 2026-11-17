#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace fusion::loader {

    struct ModProfile {
        std::string id;
        std::string name;
        std::unordered_map<std::string, bool> enabledMods;
    };

    struct ModKeybind {
        std::string action;
        std::string chord;
    };

    // Stores per-installation mod preferences, named profiles, and keybind
    // assignments. Version 1 settings are migrated in memory on load.
    class ModSettingsStore {
    public:
        bool Load(const std::filesystem::path& filePath, std::string& error);
        bool Save(std::string& error) const;

        bool IsEnabled(const std::string& modId) const;
        void SetEnabled(const std::string& modId, bool enabled);

        const std::vector<ModProfile>& GetProfiles() const;
        const std::string& GetActiveProfileId() const;
        bool CreateProfile(const std::string& id, const std::string& name,
            std::string& error);
        bool DeleteProfile(const std::string& id, std::string& error);
        bool SetActiveProfile(const std::string& id, std::string& error);

        bool SetKeybind(const std::string& action, const std::string& chord,
            std::string& error);
        bool RemoveKeybind(const std::string& action);
        const std::vector<ModKeybind>& GetKeybinds() const;

        const std::filesystem::path& GetFilePath() const;

    private:
        ModProfile* FindActiveProfile();
        const ModProfile* FindActiveProfile() const;

        std::filesystem::path filePath_;
        std::vector<ModProfile> profiles_;
        std::string activeProfileId_ = "default";
        std::vector<ModKeybind> keybinds_;
    };

}
