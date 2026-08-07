// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Resource/Resource.hpp"

#include "Engine/Core/Hash.hpp"
#include "Engine/Core/Serial.hpp"
#include "Engine/Platform/Assert.hpp"

#include <stdio.h>

uint64_t
blk::get_resource_hash(const char* stem, Resource_Type resource_type)
{
	char filename[MAX_RESOURCE_LOGICAL_PATH_SIZE];
	int written = 0;

	switch (resource_type)
	{
	case Resource_Type::MATERIAL:
		written = snprintf(filename, sizeof(filename), "%s.bmaterial", stem);
		break;
	case Resource_Type::MESH:
		written = snprintf(filename, sizeof(filename), "%s.bmesh", stem);
		break;
	case Resource_Type::SHADER:
		written = snprintf(filename, sizeof(filename), "%s.bshader", stem);
		break;
	case Resource_Type::TEXTURE:
		written = snprintf(filename, sizeof(filename), "%s.btexture", stem);
		break;
	}

	BLK_VERIFY(written > 0 && static_cast<size_t>(written) < MAX_RESOURCE_LOGICAL_PATH_SIZE);

	return hash_fnv1a(filename);
}
