target_sources(BlkEngine
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Material.hpp
			Mesh.hpp
			Shader.hpp
			Storage.hpp
			Texture.hpp
	PRIVATE
		Material.cpp
		Mesh.cpp
		Shader.cpp
		Storage.cpp
		Texture.cpp
)
