add_library(BlkUfbx)

target_sources(BlkUfbx
	PUBLIC
		FILE_SET publicHeaders
		TYPE HEADERS
		BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
		FILES
			ufbx.h
	PRIVATE
		ufbx.c
)
