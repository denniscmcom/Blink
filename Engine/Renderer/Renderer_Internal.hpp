// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include <imgui_impl_vulkan.h>

namespace blk
{
/// Gets information needed by `Editor/` regarding our Vulkan backend.
ImGui_ImplVulkan_InitInfo get_renderer_imgui_init_info();
}  // namespace blk
