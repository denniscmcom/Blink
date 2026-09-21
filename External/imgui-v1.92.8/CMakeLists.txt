add_library(BlkImGui)

find_package(Vulkan REQUIRED)

target_sources(BlkImGui
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
		FILES
			imgui.h
			imgui_impl_vulkan.h
			imgui_impl_win32.h
			imgui_stdlib.h

		FILE_SET privateHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
		FILES
			imconfig.h
			imgui_internal.h
			imstb_rectpack.h
			imstb_textedit.h
			imstb_truetype.h

	PRIVATE
		imgui.cpp
		imgui_demo.cpp
		imgui_draw.cpp
		imgui_impl_vulkan.cpp
		imgui_impl_win32.cpp
		imgui_stdlib.cpp
		imgui_tables.cpp
		imgui_widgets.cpp
)

target_link_libraries(BlkImGui
		PRIVATE
			Vulkan::Vulkan
)
