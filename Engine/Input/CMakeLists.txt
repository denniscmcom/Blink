target_sources(BlkEngine
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../
		FILES
			Input.hpp
	PRIVATE
		Input.cpp
)
