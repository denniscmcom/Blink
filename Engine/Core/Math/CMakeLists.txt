target_sources(BlkEngine
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Constants.hpp
			Matrix.hpp
			Unit.hpp
			Vector.hpp
	PRIVATE
		Matrix.cpp
		Unit.cpp
		Vector.cpp
)
