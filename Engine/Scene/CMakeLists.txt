target_sources(BlkEngine
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Directional_Light.hpp
			Point_Light.hpp
			Mesh_Instance.hpp
			Node.hpp
			Graph.hpp
			Transform.hpp
	PRIVATE
		Graph.cpp
		Mesh_Instance.cpp
		Node.cpp
		Transform.cpp
)
