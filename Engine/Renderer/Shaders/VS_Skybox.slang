// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Interface/Frame_Set.slang"
#include "Pass/Skybox.slang"

[shader("vertex")]
VS_Skybox_Output
vs_main(uint vertex_id : SV_VertexID)
{
	// We render a triangle big enough so that our screen fits in it. By using the "triangle trick", the GPU runs our
	// fragment shader once for every pixel on the screen. The scissor clips away everything outside it.
	//
	// A quad of two triangles covers the same area, but the 2x2 pixel quads along its shared diagonal are shaded
	// once for each triangle.
	//
	// The coordinates below are normalized device coordinates, Y up, before the negative viewport height flips them.
	// The hypotenuse passes exactly through (1, 1), so this is the smallest triangle with axis aligned legs that
	// still covers the whole screen.
	//
	//   (-1, 3)
	//      *
	//      | \
	//      |   \
	//      |     \
	//      +-------+ (1, 1)
	//      |       | \
	//      | screen|   \
	//      |       |     \
	//      +-------+-------*
	//   (-1, -1)         (3, -1)
	//
	// Each `uv` component is two times one bit of the vertex index: (0, 0), (2, 0) and (0, 2), which `ndc` maps to
	// the three corners above.
	float2 uv  = float2((vertex_id << 1) & 2, vertex_id & 2);
	float2 ndc = uv * 2.0 - 1.0;

	VS_Skybox_Output output = {};

	// `z = w = 1` puts the triangle exactly on the far plane after the perspective divide, so the depth test only
	// lets it through where the scene left the depth buffer at its clear value.
	output.position = float4(ndc, 1.0, 1.0);

	// Undoing the projection and the camera rotation turns each corner into the world space ray through it. The
	// translation was stripped host side, so this is a direction from the origin and the sky never parallaxes.
	//
	// It is left unnormalized because the direction is affine in `ndc`, which makes the interpolation the rasterizer
	// does exact. Normalizing here would break that, so the fragment shader normalizes instead.
	output.direction = mul(skybox_ubo.inversed_view_projection, float4(ndc, 1.0, 1.0)).xyz;

	return output;
}
