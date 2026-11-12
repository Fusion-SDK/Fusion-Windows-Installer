
#pragma once

#include <cstdint>

namespace fusion::loader
{
    struct Version
    {
        std::uint32_t major = 0;
        std::uint32_t minor = 1;
        std::uint32_t patch = 0;

        constexpr bool operator==(const Version&) const = default;
    };

    inline constexpr Version LoaderVersion{0, 1, 0};
    inline constexpr std::uint32_t LoaderApiVersion = 1;
}
