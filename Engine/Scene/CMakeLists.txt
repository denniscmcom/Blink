target_sources(BlkEngine
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
		FILES
			Light.hpp
			Mesh_Instance.hpp
			Node.hpp
			Scene_Graph.hpp
			Transform.hpp
	PRIVATE
		Scene_Graph.cpp
		Transform.cpp
)
