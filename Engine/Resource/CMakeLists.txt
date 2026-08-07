target_sources(BlkEngine
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Material.hpp
			Mesh.hpp
			Shader.hpp
		Resource.hpp
			Texture.hpp
	PRIVATE
		FILE_SET privateHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Resource_Storage.hpp
	PRIVATE
		Material.cpp
		Mesh.cpp
		Shader.cpp
		Resource.cpp
		Texture.cpp
)
