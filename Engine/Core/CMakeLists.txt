add_subdirectory("Math")

target_sources(BlkEngine
	PUBLIC
		FILE_SET HEADERS
		BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../
		FILES
			Hash.hpp
			Iterator.hpp
			Pool.hpp
			Time.hpp
	PRIVATE
		Hash.cpp
		Time.cpp
)

