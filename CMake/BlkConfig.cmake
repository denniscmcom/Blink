function(blk_set_target_config TARGET)
set_target_properties(${TARGET} PROPERTIES
		CXX_STANDARD 20
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
)

string(LENGTH "${CMAKE_SOURCE_DIR}/" BLK_SOURCE_PATH_PREFIX_LENGTH)

target_compile_definitions(${TARGET}
		PRIVATE
			_CRT_SECURE_NO_WARNINGS
			BLK_SOURCE_PATH_PREFIX_LENGTH=${BLK_SOURCE_PATH_PREFIX_LENGTH}
			BLK_ENGINE_VERSION="2026.08.08.1-Dev"
			#BLK_NEW_API # TODO (Temporary)
			$<$<CONFIG:Debug>:BLK_TARGET_DEBUG>
			$<$<CONFIG:Debug>:BLK_EDITOR>
)

if(MSVC)
	target_compile_options(${TARGET}
			PRIVATE
				/Zc:preprocessor
	)
endif()
# TODO (Consistency): Compile with UBsan and AddressSanitizer in debug.
endfunction()
