#pragma once

#include <stdint.h>

#if defined(_WIN32)
    #define FUSION_MOD_EXPORT __declspec(dllexport)
#else
    #define FUSION_MOD_EXPORT
#endif

#define FUSION_MOD_API_VERSION 1u

#ifdef __cplusplus
extern "C" {
#endif

typedef enum FusionLogLevel {
    FUSION_LOG_INFO = 0,
    FUSION_LOG_WARNING = 1,
    FUSION_LOG_ERROR = 2
} FusionLogLevel;

typedef struct FusionHostApi {
    uint32_t struct_size;
    uint32_t api_version;

    void (*log)(
        uint32_t level,
        const char* message
    );

} FusionHostApi;

typedef struct FusionModInfo {
    uint32_t struct_size;

    const char* id;
    const char* name;
    const char* version;
    const char* author;

    uint32_t required_api_version;

} FusionModInfo;

typedef int32_t (*FusionModLoadFn)(
    const FusionHostApi* host
);

typedef void (*FusionModUnloadFn)(void);

typedef struct FusionModApi {
    uint32_t struct_size;
    uint32_t api_version;

    FusionModInfo info;

    FusionModLoadFn on_load;
    FusionModUnloadFn on_unload;

} FusionModApi;

typedef const FusionModApi* (*FusionModGetApiFn)(void);

#ifdef __cplusplus
}
#endif
