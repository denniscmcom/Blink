target_sources(BlkEngine
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Editor.hpp
	PRIVATE
		FILE_SET privateHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Camera.hpp
			Console.hpp
			Context.hpp
			Menu.hpp
			Scene_Graph.hpp
			Settings.hpp
			Stats.hpp
			Status_Bar.hpp
			Toolbar.hpp
	PRIVATE
		Camera.cpp
		Console.cpp
		Editor.cpp
		Menu.cpp
		Scene_Graph.cpp
		Settings.cpp
		Stats.cpp
		Status_Bar.cpp
		Toolbar.cpp
)

target_compile_definitions(BlkEngine
	PRIVATE
		BLK_SOURCE_ASSETS_DIRECTORY="${CMAKE_SOURCE_DIR}/Assets"
		BLK_EDITOR_RESOURCES_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/Resources"
)
