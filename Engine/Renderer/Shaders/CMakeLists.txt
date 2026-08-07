find_program(SLANGC slangc REQUIRED)

function(blk_add_shaders TARGET)
	set(STAMP_FILES "")

	foreach(SHADER IN LISTS ARGN)
		string(REPLACE "|" ";" SHADER_PARTS ${SHADER})

		list(GET SHADER_PARTS 0 RELATIVE_SOURCE)
		list(GET SHADER_PARTS 1 ENTRY)

		cmake_path(GET RELATIVE_SOURCE STEM SOURCE_STEM)

		set(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${RELATIVE_SOURCE})
		set(OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_STEM}.spv)
		set(DEPEND_FILE ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_STEM}.spv.d)
		set(STAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_STEM}.stamp)

		add_custom_command(
			OUTPUT ${OUTPUT_FILE}
			COMMAND
				${SLANGC} ${SOURCE}
				-target spirv
				-profile spirv_1_4
				-emit-spirv-directly
				-fvk-use-entrypoint-name
				-entry ${ENTRY}
				-o ${OUTPUT_FILE}
				-depfile ${DEPEND_FILE}
			DEPENDS ${SOURCE}
			DEPFILE ${DEPEND_FILE}
			COMMENT "Compiling shader: ${RELATIVE_SOURCE}"
			VERBATIM
		)

		add_custom_command(
			OUTPUT ${STAMP_FILE}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${BLK_RUNTIME_DIRECTORY}/Shaders
			COMMAND BlkCompiler
				--input ${SOURCE_STEM}.spv
				--input-dir ${CMAKE_CURRENT_BINARY_DIR}
				--output-dir ${BLK_RUNTIME_DIRECTORY}/Shaders
			COMMAND ${CMAKE_COMMAND} -E touch ${STAMP_FILE}
			DEPENDS BlkCompiler ${OUTPUT_FILE}
			COMMENT "Compiling asset: ${SOURCE_STEM}.spv"
			VERBATIM
		)

		list(APPEND STAMP_FILES ${STAMP_FILE})
	endforeach()

	add_custom_target(${TARGET}
		DEPENDS
			${STAMP_FILES}
	)
endfunction()

blk_add_shaders(BlkShaders
	FS_Light.slang|fs_main
	FS_Mesh.slang|fs_main
	VS_PNC.slang|vs_main
	VS_PNT.slang|vs_main
)
