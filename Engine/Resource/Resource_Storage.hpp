#pragma once

#include "Core/Hash.hpp"
#include "Core/Pool.hpp"

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

// `stem` owns its storage: callers may pass a temporary buffer (the editor does).
struct Resource_Metadata
{
	uint64_t hash;
	std::string stem;
};

template <typename Type>
Pool_Handle<Type> store_resource(Resource_Storage<Type>& storage, Type resource, const char* stem, const char* path);
template <typename Type>
void release_resource(Resource_Storage<Type>& storage, Pool_Handle<Type> handle);
template <typename Type>
Type* get_resource(const Resource_Storage<Type>& storage, Pool_Handle<Type> handle);
template <typename Type>
Pool_Handle<Type> get_resource_handle(const Resource_Storage<Type>& storage, const char* path);
template <typename Type>
bool is_resource_loaded(const Resource_Storage<Type>& storage, const char* path);

template <typename Type>
Pool_Handle<Type>
store_resource(Resource_Storage<Type>& storage, Type resource, const char* stem, const char* path)
{
	Resource_Metadata metadata = {};
	metadata.hash = hash_fnv1a(path);
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
get_resource_handle(const Resource_Storage<Type>& storage, const char* path)
{
	const uint64_t hash = hash_fnv1a(path);
	auto search = storage.hash_to_handle.find(hash);
	BLK_CHECK(search != storage.hash_to_handle.end());

	return search->second;
}

template <typename Type>
bool
is_resource_loaded(const Resource_Storage<Type>& storage, const char* path)
{
	const uint64_t hash = hash_fnv1a(path);

	return storage.hash_to_handle.contains(hash);
}
}  // namespace blk
