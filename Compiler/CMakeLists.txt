add_executable(BlkCompiler)

include(BlkConfig)
blk_set_target_config(BlkCompiler)

target_sources(BlkCompiler
	PRIVATE
		Main.cpp
)

target_link_libraries(BlkCompiler
	PRIVATE
		BlkEngine
)
