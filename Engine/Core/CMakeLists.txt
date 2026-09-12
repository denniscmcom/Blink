target_sources(BlkEngine
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Array.hpp
			Hash.hpp
			Pool.hpp
			Serial.hpp
			String.hpp
	PRIVATE
		Hash.cpp
		Serial.cpp
		String.cpp
)

add_subdirectory(Math)

