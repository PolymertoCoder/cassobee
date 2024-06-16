#pragma once
#include <stdint.h>

using TIMETYPE = int64_t;
using SID = uint64_t;
using PROTOCOLID = uint32_t;

#define FORCE_INLINE __attribute__((always_inline))
#define ATTR_WEAK __attribute__((weak))