
#include "fusion/loader/manifest_parser.hpp"

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

            metadata.requiredLoaderApi = 1;
            metadata.dependencies.clear();

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

                    parsed.id =
                        dependency["id"].get<std::string>();

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

                    metadata.dependencies.push_back(
                        std::move(parsed)
                    );
                }
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
