#pragma once

#include "Core/Pool.hpp"
#include "Renderer/Lifetime/Image.hpp"
#include "Resource/Texture.hpp"

#include <vulkan/vulkan.h>

namespace blk
{
struct Context;
struct Arena;

struct Texture_Device
{
	Pool_Handle<Texture> handle;
	Image image;
};

Texture_Device
transfer_texture(const Context& context, Arena& arena, const Pool_Handle<Texture>& handle, VkFormat format);
}  // namespace blk
