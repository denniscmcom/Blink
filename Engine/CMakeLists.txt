add_library(BlkEngine)

include(BlkConfig)
blk_set_target_config(BlkEngine)

add_subdirectory(Core)
add_subdirectory(Editor)
add_subdirectory(Input)
add_subdirectory(Platform)
add_subdirectory(Renderer)
add_subdirectory(Resource)
add_subdirectory(Scene)
add_subdirectory(World)

find_package(Vulkan REQUIRED)

target_link_libraries(BlkEngine
		PRIVATE
			BlkStb
			BlkImGui
			BlkIconFontCppHeaders
			Vulkan::Vulkan
)

target_compile_definitions(BlkEngine
	PRIVATE
		BLK_ENGINE_VERSION="2026.08.08.1-Dev"
)
