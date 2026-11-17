#include "fusion/loader/mod_settings.hpp"

#include <algorithm>
#include <cctype>
#include <exception>
#include <fstream>
#include <nlohmann/json.hpp>
#include <system_error>
#include <unordered_set>

namespace fusion::loader {

    using json = nlohmann::json;

    namespace {
        bool IsSafeIdentifier(const std::string& value)
        {
            if (value.empty() || value.size() > 64) return false;
            return std::all_of(value.begin(), value.end(), [](unsigned char c) {
                return std::isalnum(c) || c == '.' || c == '_' || c == '-';
            });
        }

        json EnabledMapToJson(const std::unordered_map<std::string, bool>& values)
        {
            json result = json::object();
            for (const auto& [id, enabled] : values) result[id] = enabled;
            return result;
        }

        bool ParseEnabledMap(const json& value,
            std::unordered_map<std::string, bool>& output)
        {
            if (!value.is_object()) return false;
            output.clear();
            for (auto it = value.begin(); it != value.end(); ++it) {
                if (it.key().empty() || !it.value().is_boolean()) return false;
                output.emplace(it.key(), it.value().get<bool>());
            }
            return true;
        }
    }

    bool ModSettingsStore::Load(
        const std::filesystem::path& filePath, std::string& error)
    {
        error.clear();
        filePath_ = filePath;
        profiles_.clear();
        keybinds_.clear();
        activeProfileId_ = "default";

        std::error_code ec;
        if (!std::filesystem::exists(filePath_, ec)) {
            if (ec) { error = "Unable to check mod settings file: " + ec.message(); return false; }
            profiles_.push_back({"default", "Default", {}});
            return true;
        }

        std::ifstream input(filePath_);
        if (!input) { error = "Unable to open mod settings file."; return false; }

        try {
            json document;
            input >> document;
            if (!document.is_object() || !document.contains("version") ||
                !document["version"].is_number_integer()) {
                error = "Mod settings file has an unsupported format.";
                return false;
            }

            const int version = document["version"].get<int>();
            if (version == 1) {
                if (!document.contains("mods") ||
                    !ParseEnabledMap(document["mods"], profiles_.emplace_back().enabledMods)) {
                    profiles_.clear();
                    error = "Version 1 mod settings contain invalid entries.";
                    return false;
                }
                profiles_.back().id = "default";
                profiles_.back().name = "Default";
                activeProfileId_ = "default";
                return true;
            }

            if (version != 2 || !document.contains("profiles") ||
                !document["profiles"].is_array()) {
                error = "Mod settings file has an unsupported version or structure.";
                return false;
            }

            std::unordered_set<std::string> profileIds;
            for (const auto& item : document["profiles"]) {
                if (!item.is_object() || !item.contains("id") ||
                    !item["id"].is_string() || !item.contains("name") ||
                    !item["name"].is_string() || !item.contains("mods")) {
                    error = "A mod profile is malformed."; profiles_.clear(); return false;
                }
                ModProfile profile;
                profile.id = item["id"].get<std::string>();
                profile.name = item["name"].get<std::string>();
                if (!IsSafeIdentifier(profile.id) || profile.name.empty() ||
                    profile.name.size() > 80 || !profileIds.insert(profile.id).second ||
                    !ParseEnabledMap(item["mods"], profile.enabledMods)) {
                    error = "A mod profile contains invalid or duplicate values.";
                    profiles_.clear(); return false;
                }
                profiles_.push_back(std::move(profile));
            }

            if (profiles_.empty()) profiles_.push_back({"default", "Default", {}});
            if (document.contains("activeProfile") && document["activeProfile"].is_string())
                activeProfileId_ = document["activeProfile"].get<std::string>();
            if (std::none_of(profiles_.begin(), profiles_.end(), [&](const auto& p) {
                    return p.id == activeProfileId_;
                })) {
                error = "The selected mod profile does not exist.";
                profiles_.clear(); return false;
            }

            if (document.contains("keybinds")) {
                if (!document["keybinds"].is_array()) {
                    error = "Keybinds must be an array."; profiles_.clear(); return false;
                }
                std::unordered_set<std::string> actions;
                for (const auto& item : document["keybinds"]) {
                    if (!item.is_object() || !item.contains("action") ||
                        !item["action"].is_string() || !item.contains("chord") ||
                        !item["chord"].is_string()) {
                        error = "A keybind entry is malformed."; profiles_.clear(); return false;
                    }
                    ModKeybind keybind{item["action"].get<std::string>(), item["chord"].get<std::string>()};
                    if (!IsSafeIdentifier(keybind.action) || keybind.chord.empty() ||
                        keybind.chord.size() > 64 || !actions.insert(keybind.action).second) {
                        error = "A keybind contains invalid or duplicate values.";
                        profiles_.clear(); keybinds_.clear(); return false;
                    }
                    keybinds_.push_back(std::move(keybind));
                }
            }
            return true;
        }
        catch (const json::exception& exception) {
            profiles_.clear(); keybinds_.clear();
            error = std::string("Invalid mod settings JSON: ") + exception.what();
            return false;
        }
        catch (const std::exception& exception) {
            profiles_.clear(); keybinds_.clear();
            error = std::string("Unable to read mod settings: ") + exception.what();
            return false;
        }
    }

    bool ModSettingsStore::Save(std::string& error) const
    {
        error.clear();
        if (filePath_.empty()) { error = "Mod settings file path has not been configured."; return false; }

        json profiles = json::array();
        for (const auto& profile : profiles_) {
            profiles.push_back({{"id", profile.id}, {"name", profile.name},
                {"mods", EnabledMapToJson(profile.enabledMods)}});
        }
        json keybinds = json::array();
        for (const auto& keybind : keybinds_)
            keybinds.push_back({{"action", keybind.action}, {"chord", keybind.chord}});
        const json document = {{"version", 2}, {"activeProfile", activeProfileId_},
            {"profiles", std::move(profiles)}, {"keybinds", std::move(keybinds)}};

        const auto parent = filePath_.parent_path();
        std::error_code ec;
        if (!parent.empty()) {
            std::filesystem::create_directories(parent, ec);
            if (ec) { error = "Unable to create settings directory: " + ec.message(); return false; }
        }

        auto temporaryPath = filePath_; temporaryPath += ".tmp";
        { std::ofstream output(temporaryPath, std::ios::trunc);
          if (!output) { error = "Unable to create temporary settings file."; return false; }
          output << document.dump(4) << '\n'; output.flush();
          if (!output) { error = "Unable to write temporary settings file."; output.close();
              std::filesystem::remove(temporaryPath, ec); return false; } }

        auto backupPath = filePath_; backupPath += ".bak";
        const bool hadPreviousFile = std::filesystem::exists(filePath_, ec) && !ec;
        if (ec) { error = "Unable to inspect existing settings: " + ec.message();
            std::filesystem::remove(temporaryPath, ec); return false; }
        if (hadPreviousFile) {
            std::filesystem::remove(backupPath, ec);
            if (ec) { error = "Unable to prepare settings backup: " + ec.message();
                std::filesystem::remove(temporaryPath, ec); return false; }
            std::filesystem::rename(filePath_, backupPath, ec);
            if (ec) { error = "Unable to preserve existing settings: " + ec.message();
                std::filesystem::remove(temporaryPath, ec); return false; }
        }
        std::filesystem::rename(temporaryPath, filePath_, ec);
        if (ec) {
            const auto message = ec.message();
            if (hadPreviousFile) { std::error_code restore; std::filesystem::rename(backupPath, filePath_, restore); }
            error = "Unable to finalize settings file: " + message;
            std::filesystem::remove(temporaryPath, ec); return false;
        }
        if (hadPreviousFile) { std::error_code cleanup; std::filesystem::remove(backupPath, cleanup); }
        return true;
    }

    ModProfile* ModSettingsStore::FindActiveProfile()
    {
        const auto found = std::find_if(profiles_.begin(), profiles_.end(), [&](const auto& p) {
            return p.id == activeProfileId_;
        });
        return found == profiles_.end() ? nullptr : &*found;
    }

    const ModProfile* ModSettingsStore::FindActiveProfile() const
    {
        const auto found = std::find_if(profiles_.begin(), profiles_.end(), [&](const auto& p) {
            return p.id == activeProfileId_;
        });
        return found == profiles_.end() ? nullptr : &*found;
    }

    bool ModSettingsStore::IsEnabled(const std::string& modId) const
    {
        const auto* profile = FindActiveProfile();
        if (!profile) return true;
        const auto found = profile->enabledMods.find(modId);
        return found == profile->enabledMods.end() || found->second;
    }

    void ModSettingsStore::SetEnabled(const std::string& modId, bool enabled)
    {
        if (auto* profile = FindActiveProfile(); profile && !modId.empty())
            profile->enabledMods[modId] = enabled;
    }

    const std::vector<ModProfile>& ModSettingsStore::GetProfiles() const { return profiles_; }
    const std::string& ModSettingsStore::GetActiveProfileId() const { return activeProfileId_; }

    bool ModSettingsStore::CreateProfile(const std::string& id, const std::string& name, std::string& error)
    {
        error.clear();
        if (!IsSafeIdentifier(id) || name.empty() || name.size() > 80) {
            error = "Profile ID or name is invalid."; return false;
        }
        if (std::any_of(profiles_.begin(), profiles_.end(), [&](const auto& p) { return p.id == id; })) {
            error = "A profile with that ID already exists."; return false;
        }
        profiles_.push_back({id, name, {}}); return true;
    }

    bool ModSettingsStore::DeleteProfile(const std::string& id, std::string& error)
    {
        error.clear();
        if (profiles_.size() <= 1) { error = "The last remaining profile cannot be deleted."; return false; }
        const auto found = std::find_if(profiles_.begin(), profiles_.end(), [&](const auto& p) { return p.id == id; });
        if (found == profiles_.end()) { error = "Profile not found."; return false; }
        const bool deletingActive = id == activeProfileId_;
        profiles_.erase(found);
        if (deletingActive) activeProfileId_ = profiles_.front().id;
        return true;
    }

    bool ModSettingsStore::SetActiveProfile(const std::string& id, std::string& error)
    {
        error.clear();
        if (std::none_of(profiles_.begin(), profiles_.end(), [&](const auto& p) { return p.id == id; })) {
            error = "Profile not found."; return false;
        }
        activeProfileId_ = id; return true;
    }

    bool ModSettingsStore::SetKeybind(const std::string& action, const std::string& chord, std::string& error)
    {
        error.clear();
        if (!IsSafeIdentifier(action) || chord.empty() || chord.size() > 64) {
            error = "Keybind action or chord is invalid."; return false;
        }
        for (const auto& existing : keybinds_) {
            if (existing.action != action && existing.chord == chord) {
                error = "That key combination is already assigned."; return false;
            }
        }
        const auto found = std::find_if(keybinds_.begin(), keybinds_.end(), [&](const auto& k) { return k.action == action; });
        if (found == keybinds_.end()) keybinds_.push_back({action, chord});
        else found->chord = chord;
        return true;
    }

    bool ModSettingsStore::RemoveKeybind(const std::string& action)
    {
        const auto oldSize = keybinds_.size();
        std::erase_if(keybinds_, [&](const auto& k) { return k.action == action; });
        return keybinds_.size() != oldSize;
    }

    const std::vector<ModKeybind>& ModSettingsStore::GetKeybinds() const { return keybinds_; }
    const std::filesystem::path& ModSettingsStore::GetFilePath() const { return filePath_; }
}
