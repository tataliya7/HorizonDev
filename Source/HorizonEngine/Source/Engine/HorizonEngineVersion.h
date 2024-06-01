#pragma once

#define HORIZON_ENGINE_MAKE_VERSION(major, minor, patch) ((((uint32_t)(major)) << 24) | (((uint32_t)(minor)) << 16) | ((uint32_t)(patch << 8)) | 0)

#define HORIZON_ENGINE_VERSION_MAJOR(version) (((uint32_t)(version) >> 24) & 0xFFU)
#define HORIZON_ENGINE_VERSION_MINOR(version) (((uint32_t)(version) >> 16) & 0xFFU)
#define HORIZON_ENGINE_VERSION_PATCH(version) (((uint32_t)(version) >> 8) & 0xFFU)

#define HORIZON_ENGINE_VERSION HORIZON_ENGINE_MAKE_VERSION(1, 0, 0)

#define HORIZON_ENGINE_NAME "Horizon Engine"