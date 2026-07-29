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

add_custom_command(
	TARGET BlkLauncher 
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_BINARY_DIR}/Engine/Renderer/Shaders
		${CMAKE_CURRENT_BINARY_DIR}/Shaders
	COMMENT "Copying shaders to output directory..."
)

add_custom_command(
	TARGET BlkLauncher
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_SOURCE_DIR}/Assets
		${CMAKE_CURRENT_BINARY_DIR}/Assets
	COMMENT "Copying assets to output directory..."
)
