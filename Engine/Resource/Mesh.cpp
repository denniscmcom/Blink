// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Engine/Resource/Mesh.hpp"

#include "Engine/Core/Pool.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Resource/Resource_Storage.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "Engine/Core/Math/Constants.hpp"

#include <tiny_gltf.h>

#include <string>
#include <vector>

namespace
{
blk::Resource_Storage<blk::Mesh> mesh_storage = {};
}  // namespace

blk::Pool_Handle<blk::Mesh>
blk::load_mesh(const char* stem)
{
	// TODO: (performance)
	std::string path = "./Assets/Meshes/";
	path += stem;
	path += ".glb";

	if (is_resource_loaded(mesh_storage, path.c_str()))
	{
		return get_resource_handle(mesh_storage, path.c_str());
	}

	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string error;
	std::string warning;

	const bool result = loader.LoadBinaryFromFile(&model, &error, &warning, path);

	if (!warning.empty())
	{
		BLK_WARNING("%s\n", warning.c_str());
	}

	if (!error.empty())
	{
		BLK_ERROR("%s\n", error.c_str());
	}

	if (!result)
	{
		BLK_FATAL("Failed to load mesh\n");
	}

	Mesh mesh = {};

	for (const auto& gltf_mesh : model.meshes)
	{
		for (const auto& gltf_primitive : gltf_mesh.primitives)
		{
			const tinygltf::Accessor& index_accessor = model.accessors[gltf_primitive.indices];
			const tinygltf::BufferView& index_buffer_view = model.bufferViews[index_accessor.bufferView];
			const tinygltf::Buffer& index_buffer = model.buffers[index_buffer_view.buffer];

			const tinygltf::Accessor& position_accessor = model.accessors[gltf_primitive.attributes.at("POSITION")];
			const tinygltf::BufferView& position_buffer_view = model.bufferViews[position_accessor.bufferView];
			const tinygltf::Buffer& position_buffer = model.buffers[position_buffer_view.buffer];

			const bool has_texture_coords = gltf_primitive.attributes.contains("TEXCOORD_0");
			const tinygltf::Accessor* texture_coord_accessor = nullptr;
			const tinygltf::BufferView* texture_coord_buffer_view = nullptr;
			const tinygltf::Buffer* texture_coord_buffer = nullptr;

			if (has_texture_coords)
			{
				texture_coord_accessor = &model.accessors[gltf_primitive.attributes.at("TEXCOORD_0")];
				texture_coord_buffer_view = &model.bufferViews[texture_coord_accessor->bufferView];
				texture_coord_buffer = &model.buffers[texture_coord_buffer_view->buffer];
			}

			const bool has_normals = gltf_primitive.attributes.contains("NORMAL");
			const tinygltf::Accessor* normals_accessor = nullptr;
			const tinygltf::BufferView* normals_buffer_view = nullptr;
			const tinygltf::Buffer* normals_buffer = nullptr;

			if (has_normals)
			{
				normals_accessor = &model.accessors[gltf_primitive.attributes.at("NORMAL")];
				normals_buffer_view = &model.bufferViews[normals_accessor->bufferView];
				normals_buffer = &model.buffers[normals_buffer_view->buffer];
			}

			for (size_t i = 0; i < position_accessor.count; i++)
			{
				Vertex_PNT vertex = {};

				auto* position = reinterpret_cast<const float*>(
					&position_buffer.data[position_buffer_view.byteOffset + position_accessor.byteOffset + i * 12]
				);

				// glTF uses a right-handed coordinate system with Y-up.
				vertex.position.x = position[0];
				vertex.position.y = position[1];
				vertex.position.z = -position[2];

				if (has_texture_coords)
				{
					BLK_CHECK(texture_coord_buffer);
					BLK_CHECK(texture_coord_buffer_view);
					BLK_CHECK(texture_coord_accessor);

					auto* texture_coord = reinterpret_cast<const float*>(
						&texture_coord_buffer
							 ->data[texture_coord_buffer_view->byteOffset + texture_coord_accessor->byteOffset + i * 8]
					);

					vertex.texture_coord.x = texture_coord[0];
					vertex.texture_coord.y = texture_coord[1];
				}

				if (has_normals)
				{
					BLK_CHECK(normals_buffer);
					BLK_CHECK(normals_buffer_view);
					BLK_CHECK(normals_accessor);

					auto* normal = reinterpret_cast<const float*>(
						&normals_buffer->data[normals_buffer_view->byteOffset + normals_accessor->byteOffset + i * 12]
					);

					// glTF uses a right-handed coordinate system with Y-up.
					vertex.normal.x = normal[0];
					vertex.normal.y = normal[1];
					vertex.normal.z = -normal[2];
				}

				mesh.vertices.push_back(vertex);
			}

			const unsigned char* index_data =
				&index_buffer.data[index_buffer_view.byteOffset + index_accessor.byteOffset];
			const size_t index_count = index_accessor.count;
			size_t index_stride = 0;

			if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
			{
				index_stride = sizeof(uint16_t);
			}
			else if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
			{
				index_stride = sizeof(uint32_t);
			}
			else if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
			{
				index_stride = sizeof(uint8_t);
			}
			else
			{
				BLK_FATAL("Unsupported index component type");
			}

			mesh.indices.reserve(index_count);

			for (size_t i = 0; i < index_count; i++)
			{
				uint32_t index = 0;

				if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					index = *reinterpret_cast<const uint16_t*>(index_data + i * index_stride);
				}
				else if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					index = *reinterpret_cast<const uint32_t*>(index_data + i * index_stride);
				}
				else if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
				{
					index = *reinterpret_cast<const uint8_t*>(index_data + i * index_stride);
				}

				mesh.indices.push_back(index);
			}
		}
	}

	return store_resource(mesh_storage, mesh, stem, path.c_str());
}

/// `segment_count`: number of vertical slices.
/// `ring_count`: number of horizontal slices.
blk::Pool_Handle<blk::Mesh>
blk::compute_uv_sphere(const float radius, const uint32_t segment_count, const uint32_t ring_count)
{
	std::string path = "UV_Sphere:";
	path += std::to_string(radius);
	path += ":";
	path += std::to_string(segment_count);
	path += ":";
	path += std::to_string(ring_count);

	if (is_resource_loaded(mesh_storage, path.c_str()))
	{
		return get_resource_handle(mesh_storage, path.c_str());
	}

	Mesh mesh = {};

	for (uint32_t i = 0; i <= ring_count; i++)
	{
		const float phi = static_cast<float>(PI) * static_cast<float>(i) / ring_count;
		const float ring_height = radius * cosf(phi);
		const float ring_radius = radius * sinf(phi);

		for (uint32_t j = 0; j <= segment_count; j++)
		{
			const float theta = 2.0f * static_cast<float>(PI) * static_cast<float>(j) / segment_count;

			Vertex_PNT vertex = {};
			vertex.position.x = ring_radius * cosf(theta);
			vertex.position.y = ring_height;
			vertex.position.z = ring_radius * sinf(theta);

			vertex.normal = compute_unit_vector(vertex.position);

			vertex.texture_coord.x = static_cast<float>(j) / segment_count;
			vertex.texture_coord.y = static_cast<float>(i) / ring_count;

			mesh.vertices.push_back(vertex);
		}
	}

	for (uint32_t i = 0; i < ring_count; i++)
	{
		uint32_t current_ring = i * (segment_count + 1);
		uint32_t next_ring = current_ring + segment_count + 1;

		for (uint32_t j = 0; j < segment_count; j++, current_ring++, next_ring++)
		{
			if (i != 0)
			{
				mesh.indices.push_back(static_cast<Index>(current_ring));
				mesh.indices.push_back(static_cast<Index>(next_ring));
				mesh.indices.push_back(static_cast<Index>(current_ring + 1));
			}
			if (i != ring_count - 1)
			{
				mesh.indices.push_back(static_cast<Index>(current_ring + 1));
				mesh.indices.push_back(static_cast<Index>(next_ring));
				mesh.indices.push_back(static_cast<Index>(next_ring + 1));
			}
		}
	}

	return store_resource(mesh_storage, mesh, path.c_str(), path.c_str());
}

void
blk::unload_mesh(const Pool_Handle<Mesh> handle)
{
	release_resource(mesh_storage, handle);
}

blk::Mesh*
blk::get_mesh(const Pool_Handle<Mesh> handle)
{
	return get_resource(mesh_storage, handle);
}
