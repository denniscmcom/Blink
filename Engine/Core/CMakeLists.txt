add_subdirectory("Math")

target_sources(BlkEngine
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_SOURCE_DIR}
		FILES
			Hash.hpp
			Iterator.hpp
			Pool.hpp
			Serial.hpp
			Time.hpp
	PRIVATE
		Hash.cpp
		Serial.cpp
		Time.cpp
)

