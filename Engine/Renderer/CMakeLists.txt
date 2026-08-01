target_sources(BlkEngine
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Renderer.hpp
	PRIVATE
		FILE_SET privateHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Lifetime/Buffer.hpp
			Lifetime/Context.hpp
			Lifetime/Descriptor.hpp
			Lifetime/Frame.hpp
			Lifetime/Image.hpp
			Lifetime/Pipeline.hpp
			Lifetime/Swapchain.hpp
			Resource/Arena.hpp
			Resource/Material_Device.hpp
			Resource/Mesh_Device.hpp
			Resource/Texture_Device.hpp
			Helpers.hpp
			Renderer_Internal.hpp
	PRIVATE
		Lifetime/Buffer.cpp
		Lifetime/Context.cpp
		Lifetime/Descriptor.cpp
		Lifetime/Frame.cpp
		Lifetime/Image.cpp
		Lifetime/Pipeline.cpp
		Lifetime/Swapchain.cpp
		Resource/Arena.cpp
		Resource/Material_Device.cpp
		Resource/Mesh_Device.cpp
		Resource/Texture_Device.cpp
		Helpers.cpp
		Renderer.cpp
)

add_subdirectory(Shaders)
