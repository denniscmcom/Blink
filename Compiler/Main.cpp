// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Compiler/Log.hpp"
#include "Compiler/Texture.hpp"
#include "Engine/Core/Serial.hpp"
#include "Engine/Platform/Log.hpp"

#include <stdio.h>
#include <string>

int
main(int argc, char** argv)
{
	blk::create_log_sink(blk::log_console);

	if (argc != 3)
	{
		BLK_ERROR("Usage: BlkCompiler <source-path> <destination-path>\n");

		return 1;
	}

	const std::string src_path = argv[1];
	const std::string dst_path = argv[2];

	FILE* src_file = fopen(src_path.c_str(), "rb");

	if (src_file == nullptr)
	{
		BLK_FATAL("Failed to open asset: %s\n", src_path.c_str());
	}

	FILE* dst_file = fopen(dst_path.c_str(), "wb");

	if (dst_file == nullptr)
	{
		BLK_FATAL("Failed to create compiled asset: %s\n", dst_path.c_str());
	}

	if (fseek(src_file, 0, SEEK_END) != 0)
	{
		BLK_FATAL("Failed to move source file pointer to the end of the file\n");
	}

	const long src_size = ftell(src_file);

	if (fseek(src_file, 0, SEEK_SET) != 0)
	{
		BLK_FATAL("Failed to move source file pointer to the start of the file\n");
	}

	auto src_buffer = static_cast<char*>(malloc(src_size));

	if (fread(src_buffer, 1, src_size, src_file) < src_size)
	{
		BLK_FATAL("Failed to read source file\n");
	}

	blk::Serial src_serial(src_buffer, src_size);
	blk::Serial dst_serial(1'024);
	free(src_buffer);

	if (src_path.ends_with(".glb"))
	{
		// TODO:
	}
	else if (src_path.ends_with(".png"))
	{
		blk::compile_texture(src_serial, dst_serial);
	}

	if (const size_t written = fwrite(dst_serial.buffer(), 1, dst_serial.position(), dst_file);
		written != dst_serial.position() || fclose(dst_file) != 0)
	{
		BLK_FATAL("Failed to write destination buffer to file\n");
	}

	if (fclose(src_file) != 0)
	{
		BLK_FATAL("Failed to close source file\n");
	}

	return 0;
}
