
#pragma once

#include <filesystem>
#include <string>

namespace fusion::loader {

    class ModPackageManager {
    public:
        explicit ModPackageManager(
            std::filesystem::path modsDirectory
        );

        bool Install(
            const std::filesystem::path& packageDirectory,
            std::string& error
        );

        bool Uninstall(
            const std::string& modId,
            std::string& error
        );

    private:
        std::filesystem::path modsDirectory_;
    };

}
