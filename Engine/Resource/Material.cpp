// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Resource/Material.hpp"

#include "Engine/Core/Pool.hpp"
#include "Engine/Resource/Resource_Storage.hpp"
#include "Engine/Resource/Texture.hpp"

#include <string>

namespace
{
blk::Resource_Storage<blk::Material> material_storage = {};
}  // namespace

blk::Pool_Handle<blk::Material>
blk::load_material(const char* stem)
{
	std::string path = "./Assets/Materials/";
	path += stem;
	path += ".bmaterial";

	if (is_resource_loaded(material_storage, path.c_str()))
	{
		return get_resource_handle(material_storage, path.c_str());
	}

	// TODO: Parse the material from the .bmaterial file. Hardcoded to the Suzanne
	//   test maps until there is more than one material.
	Material material = {};
	material.diffuse_map = load_texture("Suzanne_Diffuse");
	material.specular_map = load_texture("Suzanne_Specular");
	material.shininess = 32.0f;

	return store_resource(material_storage, material, stem, path.c_str());
}

void
blk::unload_material(Pool_Handle<Material> handle)
{
}

blk::Material*
blk::get_material(Pool_Handle<Material> handle)
{
	return get_resource(material_storage, handle);
}
