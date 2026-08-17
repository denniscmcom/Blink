// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"
#include "Engine/Resource/Resource.hpp"
#include "Engine/Resource/Resource_Storage.hpp"

namespace blk
{
struct Texture;

struct Material
{
	Resource_Metadata metadata;
	Pool_Handle<Texture> albedo;
	Pool_Handle<Texture> normal;
	Pool_Handle<Texture> orm;
};

constexpr Magic MATERIAL_MAGIC = {'M', 'A', 'T', 'R'};
constexpr uint8_t MATERIAL_VERSION = 1;

Pool_Handle<Material> load_material(const char* stem);
void unload_material(Pool_Handle<Material> handle);
Material* get_material(Pool_Handle<Material> handle);
}  // namespace blk
