// Copyright DCM Systems. All Rights Reserved.

#pragma once

#ifdef BLK_NEW_API
#include <stddef.h>
#include <stdint.h>

namespace blk
{
struct Allocator;

// ---- Array ----

/// Fixed-size array.
template <typename Type, size_t Capacity>
struct Array
{
	/// Fixed-size data buffer.
	Type buffer[Capacity];
	/// Capacity of `buffer`.
	static constexpr size_t capacity = Capacity;
};

/// Replaces an element at `index` if `index` < `array.capacity`.
template <typename Type, size_t Capacity>
void array_replace(Array<Type, Capacity>* array, size_t index, Type item);
/// Returns the element at `index` or `nullptr` if it's out of bounds.
template <typename Type, size_t Capacity>
Type* array_at(const Array<Type, Capacity>* array, size_t index);

/// Dynamic array.
template <typename Type>
struct Dyn_Array
{
	/// Data buffer.
	Type* buffer;
	/// Live elements in `buffer`.
	size_t count;
	/// Capacity of `buffer`.
	size_t capacity;
	/// Pointer to allocator.
	Allocator* allocator;
};

/// Creates a dynamic array.
template <typename Type>
Dyn_Array<Type>* dyn_array_create(Allocator* allocator, size_t capacity);
/// Destroys a dynamic array.
template <typename Type>
void dyn_array_destroy(Dyn_Array<Type>* array);
/// Pushes an item at the end of the array.
template <typename Type>
void dyn_array_push(Dyn_Array<Type>* array, Type item);
/// Empties the array.
template <typename Type>
void dyn_array_empty(Dyn_Array<Type>* array);
/// Resizes the array only if `array.capacity` < `capacity`.
template <typename Type>
void dyn_array_resize(Dyn_Array<Type>* array, size_t capacity);
/// Replaces an element at `index` if `index` < `array.count`.
template <typename Type>
void dyn_array_replace(Dyn_Array<Type>* array, size_t index, Type item);
/// Returns the element at `index` or `nullptr` if it's out of bounds.
template <typename Type>
Type* dyn_array_at(const Dyn_Array<Type>* array, size_t index);

// ---- Hash map ----

// TODO: We should research about recommended hashing functions for open addressing hash map.
// A non-uniform distribution increases the number of collisions and the cost of resolving them. For open addressing,
// the hash function should also avoid *runs* of two or more keys to consecutive slots. Such runs may cause the lookup
// cost to skyrocket, even if the load factor is low and collisions are infrequent.
// https://en.wikipedia.org/wiki/Hash_table
// https://en.wikipedia.org/wiki/Primary_clustering

/// State of `Hash_Map` slots used by the linear probing method to resolve conflicting hashes.
enum class Hash_Map_Slot_State : uint8_t
{
	/// Slot is empty and could be used when inserting new elements.
	EMPTY,
	/// Slot is used by a live element.
	USED,
	/// Slot was lazy removed.
	REMOVED,
};

/// Hash map using FNV-1a as the hashing function and open addressing with linear probing to handle collisions. It uses
/// a lazy deletion strategy in which a key–value pair is removed by setting `Hash_Map::states` to
/// `Hash_Map_Slot_State::REMOVED`.
template <typename Key, typename Value>
struct Hash_Map
{
	/// It is calculated as `(count + removed_count) / capacity`. With open addressing, it cannot be greater than 1 and
	/// recommended range is around 0.6 to 0.75. Values closer to 0 increase performance at the cost of memory
	/// footprint. Values closer to 1 decrease memory footprint at the cost of performance.
	float max_load_factor;

	/// Unique keys of key-value association.
	Key* keys;
	/// Values of key-value association.
	Value* values;
	/// The state of each slot.
	Hash_Map_Slot_State* states;

	/// Total capacity of `keys`, `values`, and `states`.
	size_t capacity;
	/// The number of currently used slots.
	size_t count;
	/// The number of removed slots.
	size_t removed_count;
	/// Pointer to allocator.
	Allocator* allocator;
};

/// Creates a hash map.
/// @param max_load_factor It should be greater than 0 and less than 1. 0.6 <= `max_load_factor` <= 0.75 is recommended.
template <typename Key, typename Value>
Hash_Map<Key, Value> hash_map_create(Allocator* allocator, size_t capacity, float max_load_factor);
/// Destroys a hash map.
template <typename Key, typename Value>
void hash_map_destroy(Hash_Map<Key, Value>* hash_map);
/// Inserts a key-value pair into the hash_map. If `key` already exists, it replaces `value`.
template <typename Key, typename Value>
void hash_map_insert(Hash_Map<Key, Value>* hash_map, Key key, Value value);
/// Lazy removes the key–value pair from the hash map.
template <typename Key, typename Value>
void hash_map_remove(Hash_Map<Key, Value>* hash_map, Key key);
/// Gets the value of `key` or `nullptr` if it does not exist.
template <typename Key, typename Value>
Value* hash_map_get(const Hash_Map<Key, Value>* hash_map, Key key);
/// Resize the hash map only if to `hash_map.capacity` < `capacity`.
template <typename Key, typename Value>
void hash_map_resize(Hash_Map<Key, Value>* hash_map, size_t capacity);
/// Checks if `key` exists.
template <typename Key, typename Value>
bool hash_map_contains(const Hash_Map<Key, Value>* hash_map, Key key);

/// Hashes `size` bytes in `buffer` using the FNV-1a function.
uint64_t hash_fnv1a(const char* buffer, size_t size);
/// Hashes null-terminated string using the FNV-1a function.
uint64_t hash_fnv1a(const char* string);
/// Hashes a 64 bits unsigned int using the FNV-1a function.
uint64_t hash_fnv1a(uint64_t value);

// ---- Pool ----

/// Handle returned by `Pool` when inserting elements.
///
/// A lightweight, copyable handle that identifies an element in a `Pool`.
/// Handles are versioned: a handle becomes stale after its element is removed, so reusing a handle after removal will
/// not accidentally alias a newer element occupying the same slot.
template <typename>
struct Pool_Handle
{
	/// Element's position in the Pool's buffer.
	/// `SIZE_MAX` is the sentinel value that represents an invalid id.
	size_t id = SIZE_MAX;
	/// Element's slot version.
	/// `SIZE_MAX` is the sentinel value that represents an invalid version.
	size_t version = SIZE_MAX;
};

/// Internal slot used by `Pool` to track per-element occupancy and versioning.
template <typename Type>
struct Pool_Slot
{
	/// The stored element. Garbage when `is_used` is `false`.
	Type element;
	/// Whether this slot currently holds a live element.
	bool is_used;
	/// Monotonically increasing version counter. Compared against `Pool_Handle::version` to detect stale access.
	size_t version;
};

/// Pre-allocated, slot-based pool of `Type`.
///
/// Stores elements of `Type` in a flat, contiguous buffer and tracks occupancy per slot. Insertion reuses the first
/// available slot and returns a versioned handle; removal frees the slot and bumps its version so that stale handles
/// are safely detected on subsequent access.
///
/// Useful when stable, O(1) insert / remove / lookup by handle is needed and want to avoid pointer invalidation issues.
template <typename Type>
struct Pool
{
	/// Buffer of `Pool_Slot`.
	Pool_Slot<Type>* slots;
	/// Unsorted contiguous buffer of live elements in `slots`.
	Type* elements;
	/// Total capacity of `slots`.
	size_t capacity;
	/// The number of currently live elements in `slots`.
	size_t count;

	/// List of unused positions in `slots`.
	size_t* free_indices;
	/// The number of indices in `free_indices`.
	size_t free_indices_count;
	/// Pointer to allocator.
	Allocator* allocator;
};

/// Creates a pool.
template <typename Type>
Pool<Type> pool_create(Allocator* allocator, size_t capacity);
/// Destroys a pool.
template <typename Type>
void pool_destroy(Pool<Type>* pool);
/// Inserts an element and returns its handle.
template <typename Type>
Pool_Handle<Type> pool_insert(Pool<Type>* pool, Type element);
/// Removes the element identified by `handle`.
template <typename Type>
void pool_remove(Pool<Type>* pool, Pool_Handle<Type> handle);
/// Resizes the pool only if `pool.capacity` < `capacity`.
template <typename Type>
void pool_resize(Pool<Type>* pool, size_t capacity);
/// Returns a pointer to the element identified by `handle`, or `nullptr` if it does not exist.
template <typename Type>
Type* pool_get(Pool<Type>* pool, Pool_Handle<Type> handle);

// ---- Serial ----

/// Non-owning serializer/deserializer.
struct Serial
{
	/// Data buffer.
	char* buffer;
	/// Size in bytes of `buffer`.
	size_t size;
	/// Next byte position to read from or write to.
	size_t position;
};

/// Initializes a serial.
Serial serial_init(char* buffer, size_t size);
/// Reads `sizeof(Type)` from `serial.buffer` and writes the result to `data`.
template <typename Type>
void serial_read(Serial* serial, Type* data);
/// Writes `data` to `serial.buffer`. `data` should be copyable.
template <typename Type>
void serial_write(Serial* serial, Type data);
/// Writes `size` bytes from `pointer` to `serial.buffer`.
template <typename Type>
void serial_write(Serial* serial, Type* pointer, size_t size);

// ---- String ----

/// Calls `snprintf` and runs the following statement if it fails or truncates.
/// `format` needs at least one argument.
#define BLK_IF_NOT_SNPRINTF(dst, max_size, format, ...)                                                                \
	if (const int written = snprintf(dst, max_size, format, __VA_ARGS__);                                              \
		written < 0 || static_cast<size_t>(written) >= (max_size))

/// Representation of a sequence of characters.
/// It's recommended to use a stack-allocated string if you know at least the maximum size supported.
struct String
{
	/// Pointer to a null-terminated string.
	char* literal;
	/// Character count of the string without the null-termination.
	size_t length;
	/// Total allocated capacity of characters.
	size_t capacity;
	/// Pointer to allocator.
	Allocator* allocator;
};

/// Creates a string from a null-terminated literal.
String string_create(Allocator* allocator, const char* literal);
/// Creates an empty string.
String string_create(Allocator* allocator);
/// Destroys a string.
void string_destroy(String* string);
/// Resizes `string` only if `string.capacity < capacity`.
void string_resize(String* string, size_t capacity);
/// Replaces a character at `index` if `index` < `string.length`.
void string_replace(String* string, size_t index, char character);
}  // namespace blk
#else
#include "Engine/Platform/Allocator.hpp"
#include "Engine/Platform/Assert.hpp"
#include "Engine/Platform/Result.hpp"

#include <stddef.h>
#include <string.h>

/// Calls `snprintf` and runs the following statement if it fails or truncates.
/// `format` needs at least one argument.
#define BLK_IF_NOT_SNPRINTF(dst, max_size, format, ...)                                                                \
	if (const int written = snprintf(dst, max_size, format, __VA_ARGS__);                                              \
		written < 0 || static_cast<size_t>(written) >= (max_size))

namespace blk
{
/// Non-owning view into an array.
template <typename Type>
struct Array_View
{
	/// Data buffer.
	Type* buffer;
	/// Number of `Type` in `buffer`.
	size_t count;
};

/// Fixed-size array.
template <typename Type, size_t Capacity>
struct Array
{
	/// Data buffer.
	Type buffer[Capacity];
	/// Capacity of `buffer`.
	static constexpr size_t capacity = Capacity;
};

template <typename Type, size_t Capacity>
Array_View<const Type> to_array_view(const Array<Type, Capacity>& array);

/// Dynamic array.
template <typename Type>
struct Dyn_Array
{
	/// Data buffer.
	Type* buffer;
	/// Number of live `Type`.
	size_t count;
	/// Number of `Type` that fits in `buffer`.
	size_t capacity;
	/// Pointer to `Allocator`.
	Allocator* allocator;
};

/// Creates a dynamic array.
template <typename Type>
Result create_dyn_array(Dyn_Array<Type>& array, Allocator* allocator, size_t capacity);
/// Destroys a dynamic array.
template <typename Type>
void destroy_dyn_array(Dyn_Array<Type>& array);
/// Pushes a new `value` to the end of `array`.
template <typename Type>
void push(Dyn_Array<Type>& array, Type value);
/// Resizes `buffer` if `capacity > array.capacity`.
template <typename Type>
Result resize(Dyn_Array<Type>& array, size_t capacity);
/// Removes all elements from `array`.
template <typename Type>
void empty(Dyn_Array<Type>& array);
/// Sets `value` at `index`.
///
/// @warning It does not increase `array.count`, it only replaces a value at `index` with a new `value`.
template <typename Type>
Result set(Dyn_Array<Type>& array, Type value, size_t index);

// TODO: We should research about recommended hashing functions for open addressing hash map.
// A non-uniform distribution increases the number of collisions and the cost of resolving them. For open addressing,
// the hash function should also avoid *runs* of two or more keys to consecutive slots. Such runs may cause the lookup
// cost to skyrocket, even if the load factor is low and collisions are infrequent.
// https://en.wikipedia.org/wiki/Hash_table
// https://en.wikipedia.org/wiki/Primary_clustering

/// State of `Hash_Map` slots used by the linear probing method to resolve conflicting hashes.
enum class Hash_Map_Slot_State : uint8_t
{
	/// Slot is empty and could be used when inserting new elements.
	EMPTY,
	/// Slot is used by a live element.
	USED,
	/// Slot was lazy removed.
	REMOVED,
};

/// Hash map using FNV-1a as the hashing function and open addressing with linear probing to handle collisions. It uses
/// a lazy deletion strategy in which a key–value pair is removed by setting `Hash_Map::states` to
/// `Hash_Map_Slot_State::REMOVED`.
///
/// @warning `Key` should implement `uint64_t hash_fnv1a(const Key& key)` and overload the equality comparison
/// operator.
template <typename Key, typename Value>
struct Hash_Map
{
	/// It is calculated as `(count + removed_count) / capacity`. With open addressing, it cannot be greater than 1 and
	/// recommended range is around 0.6 to 0.75. Values closer to 0 increase performance at the cost of memory
	/// footprint. Values closer to 1 decrease memory footprint at the cost of performance.
	float max_load_factor;

	// We use a SoA layout to store slots.

	/// Unique keys of key-value association.
	Key* keys;
	/// Values of key-value association.
	Value* values;
	/// The state of each slot.
	Hash_Map_Slot_State* states;

	/// Total capacity of `keys`, `values`, and `states`.
	size_t capacity;
	/// The number of currently `Hash_Map_State::USED` slots.
	size_t count;
	/// The number of `Hash_Map_State::REMOVED` slots.
	size_t removed_count;
	Allocator* allocator;
};

/// Creates a `Hash_Map` with `capacity` slots and `max_load_factor`.
///
/// @param max_load_factor It should be greater than 0 and less than 1. 0.6 <= `max_load_factor` <= 0.75 is recommended.
template <typename Key, typename Value>
Result create_hash_map(Hash_Map<Key, Value>& hash_map, Allocator* allocator, size_t capacity, float max_load_factor);
/// Destroys `hash_map`.
template <typename Key, typename Value>
void destroy_hash_map(Hash_Map<Key, Value>& hash_map);
/// Inserts the association `key`–`value` into `hash_map`. If `key` already exists, it replaces `value`.
template <typename Key, typename Value>
void insert(Hash_Map<Key, Value>& hash_map, Key key, Value value);
/// Lazy removes the `key–value` pair from `hash_map`.
template <typename Key, typename Value>
void remove(Hash_Map<Key, Value>& hash_map, Key key);
/// Gets the `value` of the `key–value` pair from `hash_map`.
template <typename Key, typename Value>
Value* get(Hash_Map<Key, Value>& hash_map, Key key);
/// Resize `hash_map` to `capacity` if `capacity` > `hash_map.capacity`.
template <typename Key, typename Value>
Result resize(Hash_Map<Key, Value>& hash_map, size_t capacity);
/// Checks if `key` is in `hash_map`.
template <typename Key, typename Value>
bool contains(const Hash_Map<Key, Value>& hash_map, Key key);

/// Hashes `size` bytes in `buffer` using the FNV-1a function.
uint64_t hash_fnv1a(const char* buffer, size_t size);
/// Hashes `string` using the FNV-1a function.
/// @warning `string` should be null-terminated.
uint64_t hash_fnv1a(const char* string);
/// Hashes a 64 bits unsigned int.
uint64_t hash_fnv1a(uint64_t value);

/// Handle returned by `Pool` when inserting elements.
///
/// A lightweight, copyable handle that identifies an element in a `Pool`.
/// Handles are versioned: a handle becomes stale after its element is removed, so reusing a handle after removal will
/// not accidentally alias a newer element occupying the same slot.
template <typename>
struct Pool_Handle
{
	/// Element's position in the Pool's buffer.
	/// `SIZE_MAX` is the sentinel value that represents an invalid id.
	size_t id = SIZE_MAX;
	/// Element's slot version.
	/// `SIZE_MAX` is the sentinel value that represents an invalid version.
	size_t version = SIZE_MAX;
};

/// Hashes a `Pool_Handle`.
/// @note It is required to create `Hash_Map<Key, Value>` where `Key` is a `Pool_Handle`.
template <typename Type>
uint64_t hash_fnv1a(const Pool_Handle<Type>& key);

/// Invalid `Pool_Handle`. Compares unequal to every valid handle.
template <typename Type>
constexpr Pool_Handle<Type> POOL_HANDLE_NONE = {.id = SIZE_MAX, .version = SIZE_MAX};

/// Comparison operator for `Pool_Handle`.
/// Two handles are equal when both their `id` and `version` match.
template <typename Type>
bool operator==(const Pool_Handle<Type>& lhs, const Pool_Handle<Type>& rhs);

/// Internal slot used by `Pool` to track per-element occupancy and versioning.
template <typename Type>
struct Pool_Slot
{
	/// The stored element. Garbage when `is_used` is `false`.
	Type element;
	/// Whether this slot currently holds a live element.
	bool is_used;
	/// Monotonically increasing version counter. Compared against `Pool_Handle::version` to detect stale access.
	size_t version;
};

/// Pre-allocated, slot-based pool of `Type`.
///
/// Stores elements of `Type` in a flat, contiguous buffer and tracks occupancy per slot. Insertion reuses the first
/// available slot and returns a versioned handle; removal frees the slot and bumps its version so that stale handles
/// are safely detected on subsequent access.
///
/// Useful when stable, O(1) insert / remove / lookup by handle is needed and want to avoid pointer invalidation issues.
template <typename Type>
struct Pool
{
	/// Buffer of `Pool_Slot`.
	Pool_Slot<Type>* slots;
	/// Total capacity of `slots`.
	size_t capacity;
	/// The number of currently occupied (live) elements in `slots`.
	size_t count;

	/// List of unused positions in `slots`.
	size_t* free_indices;
	/// The number of indices in `free_indices`.
	size_t free_indices_count;
	/// Pointer to allocator.
	Allocator* allocator;
};

/// Creates a `Pool`.
template <typename Type>
Result create_pool(Pool<Type>& pool, Allocator* allocator, size_t capacity);
/// Destroys a `Pool`.
template <typename Type>
void destroy_pool(Pool<Type>& pool);
/// Inserts `element` and returns its handle, or `POOL_HANDLE_NONE` if `pool` could not grow to fit it.
template <typename Type>
Pool_Handle<Type> insert(Pool<Type>& pool, Type element);
/// Removes the element identified by `handle`. If `handle` is stale, `remove` is a no-op.
template <typename Type>
void remove(Pool<Type>& pool, const Pool_Handle<Type>& handle);
/// Resizes `buffer` if `pool.capacity < capacity`. Trying to resize to a lower `capacity` is a successful no-op.
template <typename Type>
Result resize(Pool<Type>& pool, size_t capacity);
/// Returns a pointer to the element identified by `handle`, or `nullptr` if the handle is stale.
template <typename Type>
Type* get(Pool<Type>& pool, const Pool_Handle<Type>& handle);

/// Non-owning serializer/deserializer.
struct Serial
{
	/// Data buffer.
	char* buffer = nullptr;
	/// Size in bytes of `buffer`.
	size_t size = 0;
	/// Next byte position to read from or write to.
	size_t position = 0;
};

/// Initializes `Serial`.
Serial init_serial(char* buffer, size_t size);
/// Reads `sizeof(Type)` from `serial.buffer` and writes the result to `data`.
/// @warning `Type` should be serializable.
template <typename Type>
Result read(Serial& serial, Type& data);
/// Writes `data` to `serial.buffer`.
///
/// `data` is taken by reference so that an array keeps its type. Taking it by value would let `Type` decay to a
/// pointer, and the write would store the pointer instead of what it points at – which is what `read` would then fail
/// to match, since it takes a reference already.
/// @warning `Type` should be serializable.
template <typename Type>
Result write(Serial& serial, const Type& data);
/// Writes `size` bytes from `pointer` to `serial.buffer`.
template <typename Type>
Result write(Serial& serial, Type* pointer, size_t size);

/// Representation of a sequence of characters.
///
/// @note We recommend using stack-allocated strings if you know at least the maximum size supported. Only use `String`
/// if you absolutely need a dynamic heap-allocated string.
struct String
{
	/// Pointer to a null-terminated string.
	char* literal;
	/// Character count of the string without the null-termination.
	size_t length;
	/// Total allocated capacity of characters.
	size_t capacity;
	Allocator* allocator;
};

/// Creates a `String` with `literal`. It pre-allocates double the size of the literal string passed.
/// @param literal Should be null-terminated. Its contents are copied, so it does not need to outlive `string`.
Result create_string(String& string, Allocator* allocator, const char* literal);
/// Creates an empty `String`.
Result create_string(String& string, Allocator* allocator);
/// Destroys `string`.
void destroy_string(String& string);
/// Resizes `string` if `string.capacity < capacity`. Trying to resize to a lower `capacity` is a successful no-op.
Result resize(String& string, size_t capacity);
/// Assigns `literal` to an already created `String`.
/// @param literal Should be null-terminated. Its contents are copied, so it does not need to outlive `string`.
Result assign(String& string, const char* literal);
/// Concatenates `rhs` to `lhs` so the resulting `String` is `lhs + rhs + \0`.
Result concat(String& lhs, const String& rhs);
}  // namespace blk
#endif

#include "Engine/Core/Array.inl"
#include "Engine/Core/Hash_Map.inl"
#include "Engine/Core/Pool.inl"
#include "Engine/Core/Serial.inl"
