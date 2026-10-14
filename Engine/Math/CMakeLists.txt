add_library(BlkMath)

include(BlkConfig)
blk_set_target_config(BlkMath)

target_sources(BlkMath
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Math.hpp
	PRIVATE
		Matrix.cpp
)
