target_sources(BlkEngine
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Directional_Light.hpp
			Point_Light.hpp
			Mesh_Ref.hpp
			Node.hpp
			Terrain_Ref.hpp
			Graph.hpp
			Transform.hpp
	PRIVATE
		Graph.cpp
		Transform.cpp
)
