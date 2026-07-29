#include "Shared.slang"

// FIXME: Fix these bindings to group them by usage.

[[vk::binding(0, 1)]]
ConstantBuffer<Light_UBO> light_ubo;

// FIXME: This should be (1, 1)
[[vk::binding(0, 2)]]
Sampler2D diffuse_map;

// FIXME: This should be (1, 2)
[[vk::binding(1, 2)]]
Sampler2D specular_map;

// FIXME: This should be (0,2)
[[vk::binding(2, 2)]]
ConstantBuffer<Material_UBO> material_ubo;

[shader("fragment")]
float4
fs_main(VS_PNT_Output input)
	: SV_Target
{
	// Sampled through an sRGB image format, so this is already linear.
	float3 diffuse_color = diffuse_map.Sample(input.texture_coord).rgb;
	float3 specular_color = specular_map.Sample(input.texture_coord).rgb;

	float3 normal = normalize(input.normal);
	float3 view_direction = normalize(light_ubo.view_position - input.fragment_position);

	float3 color = light_ubo.ambient_strength * diffuse_color;

	for (uint i = 0; i < light_ubo.light_count; i++)
	{
		float3 light_color = light_ubo.light_colors[i];
		float3 light_direction = normalize(light_ubo.light_positions[i] - input.fragment_position);

		float n_dot_l = dot(normal, light_direction);

		float difference = max(n_dot_l, 0.0);
		float3 diffuse = difference * light_color * diffuse_color;

		float3 reflect_direction = reflect(-light_direction, normal);
		float spec = pow(max(dot(view_direction, reflect_direction), 0.0), material_ubo.shininess);
		// Only apply specular on surfaces facing the light; otherwise a back-facing quad's
		// reflection vector can still point at the viewer and blow out to white.
		float3 specular = (n_dot_l > 0.0 ? spec : 0.0) * light_color * specular_color;

		color += diffuse + specular;
	}

	return float4(color, 1.0);
}
