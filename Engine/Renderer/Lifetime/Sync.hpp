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

/// Creates a `semaphore`.
Result create_semaphore(const Context& context, VkSemaphore& semaphore);
/// Destroys a `semaphore`.
// TODO (Bug): not implemented.
void destroy_semaphore(const Context& context, VkSemaphore& semaphore);
/// Creates a `fence`.
Result create_fence(const Context& context, VkFence& fence);
/// Destroys a `fence`.
// TODO (Bug): not implemented.
void destroy_fence(const Context& context, VkFence& fence);
}  // namespace blk
