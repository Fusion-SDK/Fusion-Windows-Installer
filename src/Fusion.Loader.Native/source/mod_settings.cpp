#include "fusion/loader/mod_settings.hpp"

#include <exception>
#include <fstream>
#include <utility>
#include <nlohmann/json.hpp>
#include <system_error>

namespace fusion::loader {

    using json = nlohmann::json;

    bool ModSettingsStore::Load(
        const std::filesystem::path& filePath,
        std::string& error)
    {
        error.clear();
        filePath_ = filePath;
        enabled_.clear();

        std::error_code ec;
        if (!std::filesystem::exists(filePath_, ec)) {
            if (ec) {
                error = "Unable to check mod settings file: " + ec.message();
                return false;
            }
            return true;
        }

        std::ifstream input(filePath_);
        if (!input) {
            error = "Unable to open mod settings file.";
            return false;
        }

        try {
            json document;
            input >> document;

            if (!document.is_object() ||
                !document.contains("version") ||
                !document["version"].is_number_integer() ||
                document["version"].get<int>() != 1 ||
                !document.contains("mods") ||
                !document["mods"].is_object()) {
                error = "Mod settings file has an unsupported format.";
                return false;
            }

            for (auto it = document["mods"].begin();
                 it != document["mods"].end(); ++it) {
                if (it.key().empty() || !it.value().is_boolean()) {
                    error = "Mod settings contain an invalid mod entry.";
                    enabled_.clear();
                    return false;
                }
                enabled_.emplace(it.key(), it.value().get<bool>());
            }

            return true;
        }
        catch (const json::exception& exception) {
            error = std::string("Invalid mod settings JSON: ") +
                exception.what();
            enabled_.clear();
            return false;
        }
        catch (const std::exception& exception) {
            error = std::string("Unable to read mod settings: ") +
                exception.what();
            enabled_.clear();
            return false;
        }
    }

    bool ModSettingsStore::Save(std::string& error) const
    {
        error.clear();

        if (filePath_.empty()) {
            error = "Mod settings file path has not been configured.";
            return false;
        }

        const auto parent = filePath_.parent_path();
        std::error_code ec;
        if (!parent.empty()) {
            std::filesystem::create_directories(parent, ec);
            if (ec) {
                error = "Unable to create mod settings directory: " +
                    ec.message();
                return false;
            }
        }

        json mods = json::object();
        for (const auto& [id, enabled] : enabled_) {
            mods[id] = enabled;
        }

        const json document = {
            {"version", 1},
            {"mods", std::move(mods)}
        };

        auto temporaryPath = filePath_;
        temporaryPath += ".tmp";

        {
            std::ofstream output(temporaryPath, std::ios::trunc);
            if (!output) {
                error = "Unable to create temporary mod settings file.";
                return false;
            }

            output << document.dump(4) << '\n';
            output.flush();
            if (!output) {
                error = "Unable to write mod settings file.";
                output.close();
                std::filesystem::remove(temporaryPath, ec);
                return false;
            }
        }

        // Keep the previous settings recoverable if replacing the file fails.
        auto backupPath = filePath_;
        backupPath += ".bak";
        const bool hadPreviousFile =
            std::filesystem::exists(filePath_, ec) && !ec;

        if (ec) {
            error = "Unable to inspect existing mod settings file: " +
                ec.message();
            std::error_code cleanupError;
            std::filesystem::remove(temporaryPath, cleanupError);
            return false;
        }

        if (hadPreviousFile) {
            std::filesystem::remove(backupPath, ec);
            if (ec) {
                error = "Unable to prepare mod settings backup: " +
                    ec.message();
                std::error_code cleanupError;
                std::filesystem::remove(temporaryPath, cleanupError);
                return false;
            }

            std::filesystem::rename(filePath_, backupPath, ec);
            if (ec) {
                error = "Unable to preserve existing mod settings: " +
                    ec.message();
                std::error_code cleanupError;
                std::filesystem::remove(temporaryPath, cleanupError);
                return false;
            }
        }

        std::filesystem::rename(temporaryPath, filePath_, ec);
        if (ec) {
            const std::string renameError = ec.message();
            if (hadPreviousFile) {
                std::error_code restoreError;
                std::filesystem::rename(backupPath, filePath_, restoreError);
            }
            error = "Unable to finalize mod settings file: " + renameError;
            std::error_code cleanupError;
            std::filesystem::remove(temporaryPath, cleanupError);
            return false;
        }

        if (hadPreviousFile) {
            std::error_code cleanupError;
            std::filesystem::remove(backupPath, cleanupError);
        }

        return true;
    }

    bool ModSettingsStore::IsEnabled(const std::string& modId) const
    {
        const auto found = enabled_.find(modId);
        return found == enabled_.end() || found->second;
    }

    void ModSettingsStore::SetEnabled(
        const std::string& modId,
        bool enabled)
    {
        if (!modId.empty()) {
            enabled_[modId] = enabled;
        }
    }

    const std::filesystem::path& ModSettingsStore::GetFilePath() const
    {
        return filePath_;
    }

}
