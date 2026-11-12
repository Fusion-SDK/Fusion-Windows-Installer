
cmake_minimum_required(VERSION 3.24)

project(FusionLoaderNative
    VERSION 0.1.0
    DESCRIPTION "Fusion native mod loader core"
    LANGUAGES CXX
)

add_library(FusionLoaderCore STATIC
    source/loader_core.cpp
)

target_include_directories(FusionLoaderCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
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
