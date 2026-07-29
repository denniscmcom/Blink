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
)

if(MSVC)
    target_compile_options(${TARGET}
            PRIVATE
                /Zc:preprocessor 
    )
endif()
endfunction()
