#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

namespace fusion::loader {

    // Stores per-mod user preferences. Mods are enabled by default unless
    // an explicit false value is saved for their ID.
    class ModSettingsStore {
    public:
        bool Load(const std::filesystem::path& filePath, std::string& error);
        bool Save(std::string& error) const;

        bool IsEnabled(const std::string& modId) const;
        void SetEnabled(const std::string& modId, bool enabled);

        const std::filesystem::path& GetFilePath() const;

    private:
        std::filesystem::path filePath_;
        std::unordered_map<std::string, bool> enabled_;
    };

}
