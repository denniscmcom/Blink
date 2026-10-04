// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/World/Terrain.hpp"

float
blk::compute_chunk_size(const Terrain_Settings& settings)
{
	if (settings.resolution == 0)
	{
		// The terrain is not configured yet.
		return 0.0f;
	}

	return settings.chunk_texel_count * (settings.width / settings.resolution);
}
