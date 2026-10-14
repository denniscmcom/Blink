add_executable(BlkLauncher WIN32)

include(BlkConfig)
blk_set_target_config(BlkLauncher)

target_sources(BlkLauncher
	PRIVATE
		Main.cpp
)

target_link_libraries(BlkLauncher
	PRIVATE
		BlkEngine
		BlkGame
		BlkImGui
)

add_dependencies(BlkLauncher BlkAssets BlkShaders)

set_target_properties(BlkLauncher PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${BLK_RUNTIME_DIRECTORY}
)

target_compile_definitions(BlkLauncher
	PRIVATE
		BLK_DEBUG_TARGET
)
