// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#pragma once

#include "Engine/Core/Pool.hpp"
#include "Engine/Platform/Assert.hpp"

#include <array>
#include <string>
#include <unordered_map>

namespace blk
{

template <typename Type>
struct Resource_Storage
{
	Pool<Type> pool;
	std::unordered_map<uint64_t, Pool_Handle<Type>> hash_to_handle;
};

// TODO: Check if `metadata` is in use.
struct Resource_Metadata
{
	uint64_t hash;
	std::string stem;
};

template <typename Type>
Pool_Handle<Type> store_resource(Resource_Storage<Type>& storage, Type resource, uint64_t hash, const char* stem);
template <typename Type>
void release_resource(Resource_Storage<Type>& storage, Pool_Handle<Type> handle);
template <typename Type>
Type* get_resource(const Resource_Storage<Type>& storage, Pool_Handle<Type> handle);
template <typename Type>
Pool_Handle<Type> get_resource_handle(const Resource_Storage<Type>& storage, uint64_t hash);
template <typename Type>
bool is_resource_loaded(const Resource_Storage<Type>& storage, uint64_t hash);

template <typename Type>
Pool_Handle<Type>
store_resource(Resource_Storage<Type>& storage, Type resource, uint64_t hash, const char* stem)
{
	Resource_Metadata metadata = {};
	metadata.hash = hash;
	metadata.stem = stem;

	resource.metadata = metadata;

	const Pool_Handle handle = storage.pool.insert(resource);
	storage.hash_to_handle.insert({metadata.hash, handle});

	return handle;
}

template <typename Type>
void
release_resource(Resource_Storage<Type>& storage, Pool_Handle<Type> handle)
{
	BLK_NOT_IMPLEMENTED();
}

template <typename Type>
Type*
get_resource(const Resource_Storage<Type>& storage, Pool_Handle<Type> handle)
{
	return storage.pool.get(handle);
}

template <typename Type>
Pool_Handle<Type>
get_resource_handle(const Resource_Storage<Type>& storage, uint64_t hash)
{
	auto search = storage.hash_to_handle.find(hash);

	if (search == storage.hash_to_handle.end())
	{
		return {};
	}

	return search->second;
}

template <typename Type>
bool
is_resource_loaded(const Resource_Storage<Type>& storage, uint64_t hash)
{
	return storage.hash_to_handle.contains(hash);
}
}  // namespace blk
