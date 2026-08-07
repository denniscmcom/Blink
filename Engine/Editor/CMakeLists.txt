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
			Helpers.hpp
			Material_Creator.hpp
			Menu.hpp
			Outliner.hpp
			Stats.hpp
			Status_Bar.hpp
	PRIVATE
		Camera.cpp
		Console.cpp
		Editor.cpp
		Helpers.cpp
		Material_Creator.cpp
		Menu.cpp
		Outliner.cpp
		Stats.cpp
		Status_Bar.cpp
)

target_compile_definitions(BlkEngine
	PRIVATE
		BLK_SOURCE_ASSETS_DIRECTORY="${CMAKE_SOURCE_DIR}/Assets"
)
