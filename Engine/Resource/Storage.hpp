// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Hash.hpp"
#include "Engine/Core/Pool.hpp"
#include "Engine/Platform/Allocator.hpp"
#include "Engine/Platform/Assert.hpp"

#include <stdio.h>

namespace blk
{
struct Serial;
struct Material;
struct Mesh;
struct Shader;
struct Texture;

/// Maximum string size for a resource stem.
constexpr size_t MAX_RESOURCE_STEM_SIZE = 50;
/// Maximum size for a resource filename.
constexpr size_t MAX_RESOURCE_FILENAME_SIZE = MAX_RESOURCE_STEM_SIZE + 50;
/// Maximum size for a resource logical path. This is usually `Assets/filename.extension`.
constexpr size_t MAX_RESOURCE_LOGICAL_PATH_SIZE = 50 + MAX_RESOURCE_FILENAME_SIZE;
/// Blink's magic number used by all engine's custom binary formats.
/// It's the general magic number. A specific magic number will follow for each resource type.
constexpr char BLINK_MAGIC[4] = {'B', 'L', 'N', 'K'};

/// The type of each supported resource.
enum class Resource_Type
{
	MATERIAL,
	MESH,
	SHADER,
	TEXTURE,
};

/// Storage when loading resources into host memory.
template <typename Type>
struct Resource_Storage
{
	/// The storage pool.
	Pool<Type> pool;
	/// Resource hash–handle association to access each resource in `pool`.
	Hash_Map<uint64_t, Pool_Handle<Type>> hash_to_handle;
};

/// Creates a resource storage of `Type`.
template <typename Type>
Result create_resource_storage(Resource_Storage<Type>& storage, Allocator* allocator);
/// Destroys a resource storage.
template <typename Type>
void destroy_resource_storage(Resource_Storage<Type>& storage);

/// Resource metadata.
///
/// This is used by `Editor/` to display the resource filename in text fields. Without it, the resource filename is lost
/// after hashing and loading it to `Resource_Storage`.
struct Resource_Metadata
{
	/// The resource hash. It is derived by its logical path.
	/// Use `get_resource_hash` to compute it.
	uint64_t hash;
	/// The stem of the resource (filename without extension).
	char stem[MAX_RESOURCE_STEM_SIZE];
};

/// Stores a resource of type `resource` in `storage`.
/// @param hash The hash of the resource. Use `get_resource_hash` to compute it.
/// @param stem A null-terminated literal string. It should be the resource's filename without extension.
template <typename Type>
Pool_Handle<Type> store_resource(Resource_Storage<Type>& storage, Type resource, uint64_t hash, const char* stem);
/// Releases a resource from `storage`.
template <typename Type>
void release_resource(Resource_Storage<Type>& storage, Pool_Handle<Type> handle);
/// Returns a pointer to a resource.
/// It may return `nullptr` if the resource `handle` does not exist in `storage`.
template <typename Type>
Type* get_resource(Resource_Storage<Type>& storage, Pool_Handle<Type> handle);
/// Returns the resource `Pool_Handle` given its hash.
/// @warning It could return `POOL_HANDLE_NONE` if there is no resource with `hash`.
/// @param hash The hash of the resource. Use `get_resource_hash` to compute it.
template <typename Type>
Pool_Handle<Type> get_resource_handle(Resource_Storage<Type>& storage, uint64_t hash);
/// Checks if a resource is already loaded in `storage`.
/// @param hash The hash of the resource. Use `get_resource_hash` to compute it.
template <typename Type>
bool is_resource_loaded(const Resource_Storage<Type>& storage, uint64_t hash);
/// Returns the resource hash given its `stem` and `resource_type`.
/// @param stem A null-terminated literal string. It should be the filename without extension and fit in
/// `MAX_RESOURCE_STEM_SIZE`.
uint64_t get_resource_hash(const char* stem, Resource_Type resource_type);
/// Writes the resource logical path given `logical_path` and returns the size of it.
/// For example, the logical path for`hash = get_resource_hash(foo, Resource_Type::MATERIAL)` is
/// `Assets/hash.bmaterial`.
/// Return the character written.
/// @param hash Resource hash generated calling `get_resource_hash`.
/// @param logical_path A stack-allocated empty char array with size `MAX_RESOURCE_LOGICAL_PATH_SIZE`.
size_t get_resource_logical_path(uint64_t hash, Resource_Type resource_type, char* logical_path);
/// Deserializes a compiled engine resource (`.bmaterial`, .`bmesh`, etc.) into `serial` and validates it.
/// The resource is valid if the file starts with `BLINK_MAGIC`.
/// @param hash The resource hash @see `get_resource_hash`.
/// @param serial The implementation will initialize a `Serial` with the size of the data and an allocated pointer that
/// points to the asset data. After using it, `serial.buffer` should be deallocated with `free(allocator,
/// serial.buffer)`. `serial.position` will be set at the start of the asset's data.
Result deserialize_resource(Allocator* allocator, Resource_Type type, uint64_t hash, Serial& serial);
}  // namespace blk

namespace blk
{
template <typename Type>
Result
create_resource_storage(Resource_Storage<Type>& storage, Allocator* allocator)
{
	if (!BLK_VERIFY(allocator))
	{
		return Result::INVALID_ARGUMENTS;
	}

	storage = {};

	BLK_SUCCESS_OR_RETURN(create_pool(storage.pool, allocator, 100));
	BLK_SUCCESS_OR_RETURN(create_hash_map(storage.hash_to_handle, allocator, 100, 0.75f));

	return Result::SUCCESS;
}

template <typename Type>
void
destroy_resource_storage(Resource_Storage<Type>& storage)
{
	destroy_pool(storage.pool);
	destroy_hash_map(storage.hash_to_handle);
}

template <typename Type>
Pool_Handle<Type>
store_resource(Resource_Storage<Type>& storage, Type resource, uint64_t hash, const char* stem)
{
	// Create `Resource_Metadata` for the resource.
	Resource_Metadata metadata = {};
	metadata.hash = hash;

	// Copy `stem` to `metadata.stem` because `Resource_Metadata` owns it.
	if (const int written = snprintf(metadata.stem, MAX_RESOURCE_STEM_SIZE, "%s", stem);
		written < 0 || static_cast<size_t>(written) >= MAX_RESOURCE_STEM_SIZE)
	{
		BLK_ERROR("Invalid resource stem\n");

		return {};
	}

	// This relies on `Type` having a `metadata` field that is of type `Resource_Metadata`.
	resource.metadata = metadata;

	// Insert `resource` into its pool.
	const Pool_Handle handle = insert(storage.pool, resource);

	// Insert the hash-handle pair.
	insert(storage.hash_to_handle, metadata.hash, handle);

	return handle;
}

template <typename Type>
void
release_resource(Resource_Storage<Type>& storage, Pool_Handle<Type> handle)
{
	// Get the hash of the resource.
	const Type* resource = get(storage.pool, handle);

	if (!resource)
	{
		// Resource does not exist, so we return early.
		return;
	}

	// Remove the hash-handle pair from the hash map. This relies on `Type` having a `metadata` field that is of type
	// `Resource_Metadata`.
	remove(storage.hash_to_handle, resource->metadata.hash);

	// Remove resource from its pool. This should be that last step.
	remove(storage.pool, handle);
}

template <typename Type>
Type*
get_resource(Resource_Storage<Type>& storage, Pool_Handle<Type> handle)
{
	return get(storage.pool, handle);
}

template <typename Type>
Pool_Handle<Type>
get_resource_handle(Resource_Storage<Type>& storage, uint64_t hash)
{
	Pool_Handle<Type>* resource_handle = get(storage.hash_to_handle, hash);

	if (!resource_handle)
	{
		// Failed to find a resource with `hash`.
		return {};
	}

	return *resource_handle;
}

template <typename Type>
bool
is_resource_loaded(const Resource_Storage<Type>& storage, uint64_t hash)
{
	return contains(storage.hash_to_handle, hash);
}
}  // namespace blk
