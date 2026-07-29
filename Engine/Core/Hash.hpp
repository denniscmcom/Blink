#pragma once

#include <stdint.h>

namespace blk
{
uint64_t hash_fnv1a(const char* buffer, size_t size);
uint64_t hash_fnv1a(const char* string);
}  // namespace blk
