#pragma once

#include <filesystem>
#include <string>

namespace fusion::loader {

    bool ComputeFileSha256(
        const std::filesystem::path& path,
        std::string& hash,
        std::string& error
    );

}
