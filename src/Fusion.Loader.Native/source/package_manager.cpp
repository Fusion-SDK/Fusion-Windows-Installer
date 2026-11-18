#include "fusion/loader/package_manager.hpp"
#include "fusion/loader/manifest_parser.hpp"
#include "fusion/loader/sha256.hpp"

#include <cctype>
#include <fstream>
#include <set>
#include <system_error>

namespace fusion::loader {

    namespace {

        bool IsSafeModId(const std::string& id)
        {
            if (id.empty() || id.size() > 128 ||
                id == "." || id == ".." ||
                id.find("..") != std::string::npos)
                return false;

            for (unsigned char character : id) {
                if (!std::isalnum(character) &&
                    character != '.' && character != '_' &&
                    character != '-')
                    return false;
            }
            return true;
        }

        bool VerifyPackageFiles(
            const std::filesystem::path& package,
            const ModMetadata& metadata,
            std::string& error)
        {
            if (metadata.packageFormatVersion == 1)
                return true;

            std::map<std::string, std::string> expected;
            for (const auto& file : metadata.files) {
                if (!expected.emplace(file.path, file.sha256).second) {
                    error = "Package file manifest contains a duplicate path.";
                    return false;
                }
            }

            std::set<std::string> verified;
            std::error_code ec;
            for (std::filesystem::recursive_directory_iterator it(package, ec), end;
                 it != end; it.increment(ec)) {
                if (ec) {
                    error = "Could not inspect package files: " + ec.message();
                    return false;
                }

                const auto status = it->symlink_status(ec);
                if (ec || std::filesystem::is_symlink(status)) {
                    error = "Symbolic links are not permitted in mod packages.";
                    return false;
                }
                if (std::filesystem::is_directory(status))
                    continue;
                if (!std::filesystem::is_regular_file(status)) {
                    error = "Package contains an unsupported file type.";
                    return false;
                }

                const auto relative =
                    std::filesystem::relative(it->path(), package, ec);
                if (ec) {
                    error = "Could not resolve package file path.";
                    return false;
                }
                const auto path = relative.generic_string();
                if (path == "mod.json")
                    continue;

                const auto entry = expected.find(path);
                if (entry == expected.end()) {
                    error = "Package contains an unlisted file: " + path;
                    return false;
                }

                std::string actual;
                if (!ComputeFileSha256(it->path(), actual, error))
                    return false;
                if (actual != entry->second) {
                    error = "SHA-256 mismatch for package file: " + path;
                    return false;
                }
                verified.insert(path);
            }

            if (verified.size() != expected.size()) {
                error = "Package is missing one or more files declared in its hash manifest.";
                return false;
            }
            return true;
        }

        bool CopyPackage(
            const std::filesystem::path& source,
            const std::filesystem::path& destination,
            std::string& error)
        {
            std::error_code ec;
            std::filesystem::create_directories(destination, ec);
            if (ec) {
                error = "Could not create staging directory: " + ec.message();
                return false;
            }

            for (const auto& entry :
                 std::filesystem::recursive_directory_iterator(source, ec))
            {
                if (ec) {
                    error = "Could not inspect package: " + ec.message();
                    return false;
                }

                const auto status = entry.symlink_status(ec);
                if (ec || std::filesystem::is_symlink(status)) {
                    error = "Symbolic links are not permitted in mod packages.";
                    return false;
                }

                const auto relative =
                    std::filesystem::relative(entry.path(), source, ec);
                if (ec || relative.empty() || relative.is_absolute()) {
                    error = "Invalid package file path.";
                    return false;
                }
                for (const auto& part : relative) {
                    if (part == "..") {
                        error = "Package contains an unsafe file path.";
                        return false;
                    }
                }

                const auto target = destination / relative;
                if (std::filesystem::is_directory(status)) {
                    std::filesystem::create_directories(target, ec);
                }
                else if (std::filesystem::is_regular_file(status)) {
                    std::filesystem::create_directories(target.parent_path(), ec);
                    if (!ec)
                        std::filesystem::copy_file(entry.path(), target,
                            std::filesystem::copy_options::none, ec);
                }
                else {
                    error = "Package contains an unsupported file type.";
                    return false;
                }

                if (ec) {
                    error = "Could not copy package file: " + ec.message();
                    return false;
                }
            }
            return true;
        }
    }

    ModPackageManager::ModPackageManager(
        std::filesystem::path modsDirectory)
        : modsDirectory_(std::move(modsDirectory))
    {
    }

    bool ModPackageManager::Install(
        const std::filesystem::path& packageDirectory,
        std::string& error)
    {
        error.clear();
        std::error_code ec;

        if (!std::filesystem::is_directory(packageDirectory, ec) || ec) {
            error = "Package directory does not exist.";
            return false;
        }

        const auto source = std::filesystem::canonical(packageDirectory, ec);
        if (ec) {
            error = "Could not resolve package directory.";
            return false;
        }

        std::filesystem::create_directories(modsDirectory_, ec);
        if (ec) {
            error = "Could not create Fusion mods directory.";
            return false;
        }

        const auto root = std::filesystem::canonical(modsDirectory_, ec);
        if (ec || source == root ||
            source.string().starts_with(root.string() +
                std::string(1, std::filesystem::path::preferred_separator))) {
            error = "Package cannot be inside the Fusion mods directory.";
            return false;
        }

        ModMetadata metadata;
        if (!ParseModManifest(source / "mod.json", metadata, error))
            return false;
        if (!IsSafeModId(metadata.id)) {
            error = "Mod ID contains invalid characters.";
            return false;
        }
        if (!VerifyPackageFiles(source, metadata, error))
            return false;

        const auto target = root / metadata.id;
        const auto staging = root / (metadata.id + ".fusion-staging");
        const auto backup = root / (metadata.id + ".fusion-backup");

        if (std::filesystem::exists(staging, ec) ||
            std::filesystem::exists(backup, ec) || ec) {
            error = "A previous installation operation needs recovery.";
            return false;
        }

        if (!CopyPackage(source, staging, error)) {
            std::filesystem::remove_all(staging, ec);
            return false;
        }

        ModMetadata stagedMetadata;
        if (!ParseModManifest(staging / "mod.json", stagedMetadata, error) ||
            stagedMetadata.id != metadata.id ||
            !VerifyPackageFiles(staging, stagedMetadata, error)) {
            std::filesystem::remove_all(staging, ec);
            error = "Staged package integrity validation failed.";
            return false;
        }

        const bool hadExisting = std::filesystem::exists(target, ec);
        if (ec) {
            std::filesystem::remove_all(staging, ec);
            error = "Could not inspect existing installation.";
            return false;
        }

        if (hadExisting) {
            if (!std::filesystem::is_directory(target, ec) || ec ||
                std::filesystem::is_symlink(std::filesystem::symlink_status(target, ec))) {
                std::filesystem::remove_all(staging, ec);
                error = "Existing mod installation is not a safe directory.";
                return false;
            }
            std::filesystem::rename(target, backup, ec);
            if (ec) {
                std::filesystem::remove_all(staging, ec);
                error = "Could not back up existing installation.";
                return false;
            }
        }

        std::filesystem::rename(staging, target, ec);
        if (ec) {
            if (hadExisting) {
                std::error_code rollbackError;
                std::filesystem::rename(backup, target, rollbackError);
            }
            std::filesystem::remove_all(staging, ec);
            error = "Could not activate new installation.";
            return false;
        }

        if (hadExisting)
            std::filesystem::remove_all(backup, ec);
        return true;
    }

    bool ModPackageManager::Uninstall(
        const std::string& modId,
        std::string& error)
    {
        error.clear();
        if (!IsSafeModId(modId)) {
            error = "Invalid mod ID.";
            return false;
        }

        std::error_code ec;
        const auto root = std::filesystem::canonical(modsDirectory_, ec);
        if (ec) {
            error = "Fusion mods directory does not exist.";
            return false;
        }

        const auto target = root / modId;
        if (!std::filesystem::exists(target, ec) || ec) {
            error = "Mod is not installed.";
            return false;
        }

        const auto status = std::filesystem::symlink_status(target, ec);
        if (ec || std::filesystem::is_symlink(status) ||
            !std::filesystem::is_directory(status)) {
            error = "Refusing to remove an unsafe mod path.";
            return false;
        }

        std::filesystem::remove_all(target, ec);
        if (ec) {
            error = "Could not remove mod: " + ec.message();
            return false;
        }
        return true;
    }

}
