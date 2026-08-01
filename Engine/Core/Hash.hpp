// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <stdint.h>

namespace blk
{
uint64_t hash_fnv1a(const char* buffer, size_t size);
uint64_t hash_fnv1a(const char* string);
}  // namespace blk
