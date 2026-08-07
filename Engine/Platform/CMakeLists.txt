target_sources(BlkEngine
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Application.hpp
			Assert.hpp
			Debug.hpp
			Event.hpp
			File.hpp
			Log.hpp
			Window.hpp
	PRIVATE
		FILE_SET privateHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Application_Internal.hpp
			Window_Internal.hpp
	PRIVATE
		Application_Win32.cpp
		Debug_Win32.cpp
		Event.cpp
		File.cpp
		Log.cpp
		Window_Win32.cpp
)
