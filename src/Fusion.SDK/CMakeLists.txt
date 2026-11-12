add_library(FusionSDK INTERFACE)

add_library(Fusion::SDK ALIAS FusionSDK)

target_include_directories(FusionSDK
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(FusionSDK
    INTERFACE
        cxx_std_20
)
