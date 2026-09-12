// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Compiler/Log.hpp"
#include "Compiler/Material.hpp"
#include "Compiler/Mesh.hpp"
#include "Compiler/Shader.hpp"
#include "Compiler/Texture.hpp"
#include "Engine/Core/Hash.hpp"
#include "Engine/Core/Serial.hpp"
#include "Engine/Platform/File.hpp"
#include "Engine/Platform/Log.hpp"
#include "Engine/Resource/Storage.hpp"

#include <stdio.h>
#include <string.h>

/// Compiles an authoring asset (FBX, PNG, etc) to the engine format.
///
/// --input: path to the asset to compile – use `/` as folder separator.
/// --output-dir: the runtime directory. The compiled asset is written to `<output-dir>/Assets/<hash>.<extension>`.
///
/// The output filename is the hash of the asset's stem plus the correspondent engine extension.
///
/// For example, given this file structure:
/// Project/
/// -- Assets/
///	  -- Textures/
///		 -- Foo.png
/// -- Meshes/
/// -- Source/
///
/// We compile Foo.png like this:
/// BlkCompiler --input Project/Assets/Textures/Foo.png --output-dir Build/Runtime
///
/// TODO (Bug): The resource hash is derived from the stem alone, so two assets of the same type with the same filename
/// in different folders – `Props/Crate.fbx` and `Level/Crate.fbx` – hash identically and the second one compiled
/// silently overwrites the first. Hashing the path relative to the asset root instead would fix it, at the cost of a
/// change in `Resource/Storage.hpp` and a full asset recompile.
///
/// An input whose extension we do not recognise is skipped with a warning rather than treated as an error. The build
/// passes every file under the asset folders, and those also hold authoring files – `.blend`, `.spp` – that have no
/// runtime format.
int
main(int argc, char** argv)
{
	// Create stdout sink to log.
	BLK_IF_NOT_SUCCESS(blk::create_log_sink(blk::log_console))
	{
		// We have no sink to report this through, so we just fail.
		return 1;
	}

	// Verify flags.

	if (argc != 5)
	{
		BLK_FATAL("Usage: BlkCompiler --input <> --output-dir <>\n");
	}

	if (strcmp(argv[1], "--input") != 0)
	{
		BLK_FATAL("Expected --input\n");
	}

	// Path to the input file. We derive the resource stem from it.
	const char* input = argv[2];

	if (strcmp(argv[3], "--output-dir") != 0)
	{
		BLK_FATAL("Expected --output-dir\n");
	}

	// The runtime directory to write the compiled asset into.
	const char* output_dir = argv[4];

	// We need the asset stem and file extension to know which file format we expect.

	// Current index to iterate `input`.
	size_t input_index = 0;

	// The index where the stem starts. There could be many nested folders like `Textures/Bar/Foo.png`, so it is the
	// character after the last separator, or 0 when `input` has none.
	size_t stem_start_index = 0;

	// Index to where the file extension starts.
	size_t extension_start_index = 0;

	while (input[input_index] != '\0')
	{
		if (input[input_index] == '/')
		{
			stem_start_index = input_index + 1;
		}

		if (input[input_index] == '.')
		{
			extension_start_index = input_index;
		}

		input_index += 1;
	}

	if (extension_start_index <= stem_start_index)
	{
		BLK_FATAL("Input `%s` has no stem or no extension\n", input);
	}

	const char* input_extension = &input[extension_start_index + 1];

	// Get the resource type from the file extension. We do this before opening anything, so that an unsupported file
	// is skipped without reading it into memory.

	auto resource_type = blk::Resource_Type::MATERIAL;
	bool is_supported_format = false;

	if (strcmp(input_extension, "bmaterial") == 0)
	{
		resource_type = blk::Resource_Type::MATERIAL;
		is_supported_format = true;
	}
	else if (strcmp(input_extension, "png") == 0)
	{
		resource_type = blk::Resource_Type::TEXTURE;
		is_supported_format = true;
	}
	else if (strcmp(input_extension, "fbx") == 0)
	{
		resource_type = blk::Resource_Type::MESH;
		is_supported_format = true;
	}
	else if (strcmp(input_extension, "spv") == 0)
	{
		resource_type = blk::Resource_Type::SHADER;
		is_supported_format = true;
	}

	if (!is_supported_format)
	{
		BLK_WARNING("Skipping `%s`: `%s` is not a format we compile\n", input, input_extension);

		return 0;
	}

	// Check input stem size against engine constraints.

	const size_t input_stem_size = extension_start_index - stem_start_index;

	if (blk::MAX_RESOURCE_STEM_SIZE <= input_stem_size)
	{
		BLK_FATAL("Input stem is longer than expected\n");
	}

	// Copy stem into its own buffer.
	char input_stem[blk::MAX_RESOURCE_STEM_SIZE];
	memcpy(input_stem, &input[stem_start_index], input_stem_size);
	input_stem[input_stem_size] = '\0';

	// Create allocator.
	//
	// The arena backs the input file buffer, the output file buffer, and every intermediate array the `compile_*`
	// functions allocate. It never reclaims what it hands out, so we size it generously rather than tightly. This is an
	// offline tool that compiles a single asset per invocation, so the memory is not worth economising.
	constexpr size_t allocator_capacity = 128 * 1'024 * 1'024;

	blk::Allocator allocator = {};

	BLK_IF_NOT_SUCCESS(blk::create_arena_allocator(allocator, allocator_capacity))
	{
		BLK_FATAL("Failed to create allocator\n");
	}

	// Open input file.

	blk::File* input_file = nullptr;

	BLK_IF_NOT_SUCCESS(blk::open_file(&allocator, input, blk::File_Access_Mode::READ, input_file))
	{
		BLK_FATAL("Failed to open input file `%s`\n", input);
	}

	// Get input file size.

	uint64_t input_file_size = 0;

	BLK_IF_NOT_SUCCESS(blk::get_file_size(input_file, input_file_size))
	{
		BLK_FATAL("Failed to get input file size\n");
	}

	// Allocate buffer to read input file.

	void* file_buffer_pointer = nullptr;

	BLK_IF_NOT_SUCCESS(blk::allocate(allocator, file_buffer_pointer, input_file_size, alignof(char)))
	{
		BLK_FATAL("Failed to allocate input file buffer\n");
	}

	auto* file_buffer = static_cast<char*>(file_buffer_pointer);

	BLK_IF_NOT_SUCCESS(blk::read_file(input_file, file_buffer, input_file_size))
	{
		BLK_FATAL("Failed to read input file\n");
	}

	// Initialize a `Serial` with the input file buffer to read its contents.

	blk::Serial input_serial = blk::init_serial(file_buffer, input_file_size);

	// We can close the input file now.

	BLK_IF_NOT_SUCCESS(blk::close_file(input_file))
	{
		BLK_FATAL("Failed to close input file\n");
	}

	// Allocate a buffer for the compiled file.

	void* output_buffer_pointer = nullptr;

	// The `compile_*` functions cannot resize this buffer yet, so it is a fixed size that fits any asset we compile. It
	// cannot be derived from the input size: a compressed input says nothing about how big its decoded form is – a flat
	// 1024x1024 PNG is a few kilobytes on disk and 4 MiB once decoded.
	constexpr size_t output_buffer_size = 64 * 1'024 * 1'024;

	BLK_IF_NOT_SUCCESS(blk::allocate(allocator, output_buffer_pointer, output_buffer_size, alignof(char)))
	{
		BLK_FATAL("Failed to allocate output file buffer\n");
	}

	blk::Serial output_serial = blk::init_serial(static_cast<char*>(output_buffer_pointer), output_buffer_size);

	// Compile the asset.

	switch (resource_type)
	{
	case blk::Resource_Type::MATERIAL:
		blk::compiler::compile_material(input_serial, output_serial);
		break;
	case blk::Resource_Type::MESH:
		blk::compiler::compile_mesh(&allocator, input_serial, output_serial);
		break;
	case blk::Resource_Type::SHADER:
		blk::compiler::compile_shader(input_serial, output_serial);
		break;
	case blk::Resource_Type::TEXTURE:
		blk::compiler::compile_texture(input_serial, output_serial);
		break;
	}

	// Get hash and logical resource path.

	const uint64_t resource_hash = blk::get_resource_hash(input_stem, resource_type);

	char resource_logical_path[blk::MAX_RESOURCE_LOGICAL_PATH_SIZE];

	if (blk::get_resource_logical_path(resource_hash, resource_type, resource_logical_path) == 0)
	{
		BLK_FATAL("Failed to get the logical path for resource `%s`\n", input_stem);
	}

	// Append `resource_logical_path` to the caller's `output_dir` flag. `resource_logical_path` already starts with
	// `Assets/`, which is what the runtime looks for.

	char output_path[blk::MAX_PATH_SIZE];

	if (const int written = snprintf(output_path, blk::MAX_PATH_SIZE, "%s/%s", output_dir, resource_logical_path);
		written < 0 || static_cast<size_t>(written) >= blk::MAX_PATH_SIZE)
	{
		BLK_FATAL("Failed to format output file path\n");
	}

	// Open output file to write to.

	blk::File* output_file = nullptr;

	BLK_IF_NOT_SUCCESS(blk::open_file(&allocator, output_path, blk::File_Access_Mode::WRITE, output_file))
	{
		BLK_FATAL("Failed to open output file `%s` to write to\n", output_path);
	}

	// Write the output file.

	BLK_IF_NOT_SUCCESS(blk::write_file(output_file, output_serial.buffer, output_serial.position))
	{
		BLK_FATAL("Failed to write output file\n");
	}

	// We can close the output file now.

	BLK_IF_NOT_SUCCESS(blk::close_file(output_file))
	{
		BLK_FATAL("Failed to close output file\n");
	}

	return 0;
}
