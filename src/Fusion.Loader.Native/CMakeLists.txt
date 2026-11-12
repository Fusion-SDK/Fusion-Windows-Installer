
cmake_minimum_required(VERSION 3.24)

project(FusionLoaderNative
    VERSION 0.3.0
    DESCRIPTION "Fusion native mod loader core"
    LANGUAGES CXX
)

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)

FetchContent_MakeAvailable(nlohmann_json)

add_library(FusionLoaderCore STATIC
    source/loader_core.cpp
    source/manifest_parser.cpp
    source/mod_registry.cpp
    source/mod_loader.cpp
)

target_include_directories(FusionLoaderCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(FusionLoaderCore
    PRIVATE nlohmann_json::nlohmann_json
)

target_compile_features(FusionLoaderCore PUBLIC cxx_std_20)

set_target_properties(FusionLoaderCore PROPERTIES
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "FusionLoaderCore"
)

if(MSVC)
    target_compile_options(FusionLoaderCore PRIVATE /W4 /permissive-)
else()
    target_compile_options(FusionLoaderCore PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()
