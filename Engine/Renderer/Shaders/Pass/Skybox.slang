// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

/// Data `VS_Skybox.slang` passes to `FS_Skybox.slang`.
struct VS_Skybox_Output
{
	float4 position : SV_Position;
	float3 direction : TEXCOORD0;
};
