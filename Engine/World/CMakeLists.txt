target_sources(BlkEngine
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
		FILES
			Actor.hpp
			Camera.hpp
			Prop.hpp
			World.hpp
	PRIVATE
		Camera.cpp
		World.cpp
)
