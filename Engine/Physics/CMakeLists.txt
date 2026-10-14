target_sources(BlkEngine
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Body.hpp
			Collider.hpp
			World.hpp
	PRIVATE
		Body.cpp
		Collider.cpp
		World.cpp
)
