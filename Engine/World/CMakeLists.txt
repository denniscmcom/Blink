target_sources(BlkEngine
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Actor.hpp
			Atmosphere.hpp
			Camera.hpp
			Chunk.hpp
			Prop.hpp
			Stream.hpp
			Terrain.hpp
			World.hpp
	PRIVATE
		Camera.cpp
		Terrain.cpp
		World.cpp
)
