#include "fusion/loader/manifest_parser.hpp"

#include <cctype>
#include <fstream>
#include <nlohmann/json.hpp>

namespace fusion::loader {

    using json = nlohmann::json;

    bool ParseModManifest(
        const std::filesystem::path& manifestPath,
        ModMetadata& metadata,
        std::string& error)
    {
        try {
            std::ifstream file(manifestPath);

            if (!file) {
                error = "Unable to open mod.json.";
                return false;
            }

            json document;
            file >> document;

            if (!document.is_object()) {
                error = "Manifest must contain a JSON object.";
                return false;
            }

            if (!document.contains("id") ||
                !document["id"].is_string() ||
                !document.contains("name") ||
                !document["name"].is_string() ||
                !document.contains("author") ||
                !document["author"].is_string() ||
                !document.contains("version") ||
                !document["version"].is_string())
            {
                error = "Manifest is missing required string fields.";
                return false;
            }

            metadata.id = document["id"].get<std::string>();
            metadata.name = document["name"].get<std::string>();
            metadata.author = document["author"].get<std::string>();
            metadata.version = document["version"].get<std::string>();

            metadata.library.clear();
            if (document.contains("library")) {
                if (!document["library"].is_string()) {
                    error = "library must be a relative path string.";
                    return false;
                }
                metadata.library = document["library"].get<std::string>();
            }

            metadata.requiredLoaderApi = 1;
            metadata.packageFormatVersion = 1;
            metadata.dependencies.clear();
            metadata.files.clear();

            if (document.contains("requiredLoaderApi")) {
                if (!document["requiredLoaderApi"].is_number_unsigned()
                    && !document["requiredLoaderApi"].is_number_integer())
                {
                    error = "requiredLoaderApi must be an integer.";
                    return false;
                }

                const auto api =
                    document["requiredLoaderApi"].get<std::int64_t>();

                if (api <= 0 || api > UINT32_MAX) {
                    error = "requiredLoaderApi is out of range.";
                    return false;
                }

                metadata.requiredLoaderApi =
                    static_cast<std::uint32_t>(api);
            }

            if (document.contains("packageFormatVersion")) {
                if (!document["packageFormatVersion"].is_number_unsigned()
                    && !document["packageFormatVersion"].is_number_integer()) {
                    error = "packageFormatVersion must be an integer.";
                    return false;
                }
                const auto format =
                    document["packageFormatVersion"].get<std::int64_t>();
                if (format != 1 && format != 2) {
                    error = "Unsupported package format version.";
                    return false;
                }
                metadata.packageFormatVersion =
                    static_cast<std::uint32_t>(format);
            }

            if (document.contains("dependencies")) {
                if (!document["dependencies"].is_array()) {
                    error = "Dependencies must be an array.";
                    return false;
                }

                for (const auto& dependency :
                     document["dependencies"])
                {
                    if (!dependency.is_object() ||
                        !dependency.contains("id") ||
                        !dependency["id"].is_string())
                    {
                        error = "A dependency has an invalid ID.";
                        return false;
                    }

                    ModDependency parsed;
                    parsed.id = dependency["id"].get<std::string>();

                    if (dependency.contains("minimumVersion")) {
                        if (!dependency["minimumVersion"].is_string()) {
                            error = "Dependency minimumVersion must be a string.";
                            return false;
                        }
                        parsed.minimumVersion =
                            dependency["minimumVersion"].get<std::string>();
                    }

                    if (parsed.id.empty()) {
                        error = "A dependency has an empty ID.";
                        return false;
                    }

                    metadata.dependencies.push_back(std::move(parsed));
                }
            }

            if (metadata.packageFormatVersion == 2) {
                if (!document.contains("files") || !document["files"].is_array()
                    || document["files"].empty()) {
                    error = "Format 2 packages require a non-empty files array.";
                    return false;
                }

                for (const auto& item : document["files"]) {
                    if (!item.is_object() ||
                        !item.contains("path") || !item["path"].is_string() ||
                        !item.contains("sha256") || !item["sha256"].is_string()) {
                        error = "Each file entry requires string path and sha256 fields.";
                        return false;
                    }

                    ModFileHash entry;
                    entry.path = item["path"].get<std::string>();
                    entry.sha256 = item["sha256"].get<std::string>();

                    const std::filesystem::path relative(entry.path);
                    if (entry.path.empty() || relative.is_absolute() ||
                        relative.has_root_name() || relative.has_root_directory() ||
                        relative.generic_string() != entry.path ||
                        entry.path == "mod.json") {
                        error = "Package file manifest contains an unsafe path.";
                        return false;
                    }
                    for (const auto& part : relative) {
                        if (part == "." || part == "..") {
                            error = "Package file manifest contains an unsafe path.";
                            return false;
                        }
                    }

                    if (entry.sha256.size() != 64 ||
                        !std::all_of(entry.sha256.begin(), entry.sha256.end(),
                            [](unsigned char c) {
                                return std::isdigit(c) ||
                                    (c >= 'a' && c <= 'f');
                            })) {
                        error = "Package file SHA-256 must be 64 lowercase hexadecimal characters.";
                        return false;
                    }

                    metadata.files.push_back(std::move(entry));
                }
            }
            else if (document.contains("files")) {
                error = "The files hash manifest is only supported by package format 2.";
                return false;
            }

            if (!IsValidMetadata(metadata)) {
                error = "Manifest metadata is incomplete.";
                return false;
            }

            return true;
        }
        catch (const json::exception& exception) {
            error = std::string("Invalid JSON manifest: ") +
                exception.what();
            return false;
        }
        catch (const std::exception& exception) {
            error = std::string("Manifest error: ") +
                exception.what();
            return false;
        }
    }

}
