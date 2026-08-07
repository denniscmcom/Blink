add_executable(BlkCompiler)

include(BlkConfig)
blk_set_target_config(BlkCompiler)

target_sources(BlkCompiler
	PRIVATE
		FILE_SET privateHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
		FILES
			Log.hpp
			Material.hpp
			Mesh.hpp
			Shader.hpp
			Texture.hpp
	PRIVATE
		Log.cpp
		Main.cpp
		Material.cpp
		Mesh.cpp
		Shader.cpp
		Texture.cpp
)

target_link_libraries(BlkCompiler
	PRIVATE
		BlkEngine
		BlkStb
		BlkUfbx
)
