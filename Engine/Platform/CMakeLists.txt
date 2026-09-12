target_sources(BlkEngine
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Allocator.hpp
			Application.hpp
			Assert.hpp
			Debug.hpp
			Event.hpp
			File.hpp
			Log.hpp
			Result.hpp
			Time.hpp
			Types.hpp
			Window.hpp
	PRIVATE
		FILE_SET privateHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Application_Internal.hpp
			Window_Internal.hpp
	PRIVATE
		Allocator.cpp
		Application_Win32.cpp
		Debug_Win32.cpp
		Event.cpp
		File_Win32.cpp
		Log.cpp
		Time_Win32.cpp
		Window_Win32.cpp
)
