// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <vulkan/vulkan.h>

namespace blk
{
struct Context;
enum class Result;

/// Creates a command `buffer` from a `pool`.
Result create_command_buffer(const Context& context, VkCommandPool pool, VkCommandBuffer& buffer);
/// Destroys a command `buffer`.
/// @param pool The same pool `buffer` was created from.
void destroy_command_buffer(const Context& context, VkCommandPool pool, VkCommandBuffer& buffer);
}  // namespace blk
