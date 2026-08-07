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

#include <format>
#include <string>
#include <string_view>

int
main(int argc, char** argv)
{
	blk::create_log_sink(blk::log_console);

	if (argc != 7)
	{
		BLK_FATAL("Usage: BlkCompiler --input <> --input-dir <> --output-dir <>\n");
	}

	if (strcmp(argv[1], "--input") != 0)
	{
		BLK_FATAL("Expected --input\n");
	}

	const std::string_view input = argv[2];
	const size_t input_last_separator = input.find_last_of("/\\");
	const size_t input_filename_start = input_last_separator == std::string_view::npos ? 0 : input_last_separator + 1;
	const std::string_view input_filename = input.substr(input_filename_start);
	const size_t input_filename_start_extension = input_filename.find_first_of('.');

	if (input_filename_start_extension == std::string_view::npos)
	{
		BLK_FATAL("Input has no file extension: %s\n", argv[2]);
	}

	const std::string_view input_stem = input_filename.substr(0, input_filename_start_extension);

	if (strcmp(argv[3], "--input-dir") != 0)
	{
		BLK_FATAL("Expected --input-dir\n");
	}

	const std::string_view input_dir = argv[4];
	const std::string input_path = std::format("{}/{}", input_dir, input);

	if (strcmp(argv[5], "--output-dir") != 0)
	{
		BLK_FATAL("Expected --output-dir\n");
	}

	const std::string_view output_dir = argv[6];
	auto logical_output_filename = std::string(input_stem);

	blk::File* input_file = blk::open_file(input_path.c_str(), blk::File_Access_Mode::READ);

	if (input_file == nullptr)
	{
		BLK_FATAL("Failed to open asset: %s\n", input_path.c_str());
	}

	const uint64_t input_file_size = blk::get_file_size(input_file);
	auto* input_buffer = static_cast<char*>(malloc(input_file_size));
	blk::read_file(input_file, input_buffer, input_file_size);

	blk::Serial input_serial(input_buffer, input_file_size);
	free(input_buffer);

	blk::Serial output_serial(1'024);
	std::string output_file_extension = "";

	if (input.ends_with(".fbx"))
	{
		output_file_extension += ".bmesh";
		blk::compile_mesh(input_serial, output_serial);
	}
	else if (input.ends_with(".png"))
	{
		output_file_extension += ".btexture";
		blk::compile_texture(input_serial, output_serial);
	}
	else if (input.ends_with(".spv"))
	{
		output_file_extension += ".bshader";
		blk::compile_shader(input_serial, output_serial);
	}
	else if (input.ends_with(".bmaterial"))
	{
		// TODO: In the future, `Editor/Material_Creator.cpp` should output the hashed file directly.
		output_file_extension += ".bmaterial";
		blk::compile_material(input_serial, output_serial);
	}
	else
	{
		BLK_FATAL("Unknown asset: %s\n", argv[2]);
	}

	logical_output_filename += output_file_extension;
	const uint64_t output_filename_hash = blk::hash_fnv1a(logical_output_filename.c_str());
	const std::string output_path = std::format("{}/{}{}", output_dir, output_filename_hash, output_file_extension);
	blk::File* output_file = blk::open_file(output_path.c_str(), blk::File_Access_Mode::WRITE);

	if (output_file == nullptr)
	{
		BLK_FATAL("Failed to create compiled asset: %s\n", output_path.c_str());
	}

	blk::write_file(output_file, output_serial.buffer(), output_serial.position());

	blk::close_file(output_file);
	blk::close_file(input_file);

	return 0;
}
