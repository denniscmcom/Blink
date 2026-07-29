find_program(SLANGC slangc REQUIRED)

function(blk_add_shaders TARGET)
	set(OUTPUT_FILES "")

	foreach(SHADER IN LISTS ARGN)
		string(REPLACE "|" ";" SHADER_PARTS ${SHADER})
		list(GET SHADER_PARTS 0 SOURCE)
		list(GET SHADER_PARTS 1 ENTRY)

		cmake_path(GET SOURCE STEM SOURCE_STEM)
		set(OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_STEM}.spv)

		add_custom_command(
			OUTPUT ${OUTPUT_FILE}
			COMMAND
				${SLANGC}
				${SOURCE}
				-target spirv
				-profile spirv_1_4
				-emit-spirv-directly
				-fvk-use-entrypoint-name
				-entry ${ENTRY}
				-o ${OUTPUT_FILE}
				DEPENDS ${SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/Shared.slang
				COMMENT "Compiling shader: ${SOURCE}"
				VERBATIM

		)

		list(APPEND OUTPUT_FILES ${OUTPUT_FILE})
	endforeach()

	add_custom_target(${TARGET} 
		DEPENDS
			${OUTPUT_FILES}
	)

endfunction()

blk_add_shaders(BlkShaders
	${CMAKE_CURRENT_SOURCE_DIR}/FS_Light.slang|fs_main
	${CMAKE_CURRENT_SOURCE_DIR}/FS_Mesh.slang|fs_main
	${CMAKE_CURRENT_SOURCE_DIR}/VS_PNC.slang|vs_main
	${CMAKE_CURRENT_SOURCE_DIR}/VS_PNT.slang|vs_main
)

add_dependencies(BlkEngine BlkShaders)
