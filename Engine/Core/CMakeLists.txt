add_library(BlkCore)

include(BlkConfig)
blk_set_target_config(BlkCore)

target_sources(BlkCore
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Array.inl
			Core.hpp
			Hash_Map.inl
			Pool.inl
			Serial.inl
	PRIVATE
		Hash.cpp
		Serial.cpp
		String.cpp
)

add_subdirectory(Math)
