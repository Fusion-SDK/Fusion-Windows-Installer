
cmake_minimum_required(VERSION 3.24)

project(FusionLoaderNative
    VERSION 0.4.0
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
    source/mod_settings.cpp
    source/mod_runtime.cpp
    source/package_manager.cpp
)

target_include_directories(FusionLoaderCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(FusionLoaderCore
    PRIVATE nlohmann_json::nlohmann_json
)

if(NOT WIN32)
    target_link_libraries(FusionLoaderCore PRIVATE ${CMAKE_DL_LIBS})
endif()

target_include_directories(FusionLoaderCore
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../Fusion.SDK/include
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


# Fusion's native bootstrap interface.
# This DLL exposes a stable C interface for the game integration layer.

add_library(FusionBootstrap SHARED
    source/bootstrap_api.cpp
    source/bootstrap_entry.cpp
)

target_include_directories(FusionBootstrap
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(FusionBootstrap
    PRIVATE
        FusionLoaderCore
)

target_compile_features(FusionBootstrap
    PRIVATE
        cxx_std_20
)

set_target_properties(FusionBootstrap PROPERTIES
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "FusionBootstrap"
)

if(MSVC)
    target_compile_options(FusionBootstrap PRIVATE /W4 /permissive-)
else()
    target_compile_options(FusionBootstrap PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()



if(WIN32)
    add_executable(FusionLauncher
        source/launcher.cpp
    )
    target_compile_features(FusionLauncher PRIVATE cxx_std_20)
    set_target_properties(FusionLauncher PROPERTIES
        CXX_EXTENSIONS OFF
        OUTPUT_NAME "Fusion.Launcher"
    )
    if(MSVC)
        target_compile_options(FusionLauncher PRIVATE /W4 /permissive-)
    else()
        target_compile_options(FusionLauncher PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()
