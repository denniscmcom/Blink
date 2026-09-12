target_sources(BlkEngine
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Color.hpp
			Constants.hpp
			Matrix.hpp
			Unit.hpp
			Vector.hpp
	PRIVATE
		Color.cpp
		Matrix.cpp
		Unit.cpp
		Vector.cpp
)
