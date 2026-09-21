function(blk_add_assets TARGET)
	set(STAMP_FILES "")

	foreach(RELATIVE_SOURCE IN LISTS ARGN)
		set(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${RELATIVE_SOURCE})
		set(STAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/${RELATIVE_SOURCE}.stamp)

		add_custom_command(
			OUTPUT ${STAMP_FILE}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${BLK_RUNTIME_DIRECTORY}/Assets
			COMMAND BlkCompiler
				--input ${SOURCE}
				--output-dir ${BLK_RUNTIME_DIRECTORY}
			COMMAND ${CMAKE_COMMAND} -E touch ${STAMP_FILE}
			DEPENDS BlkCompiler ${SOURCE}
			COMMENT "Compiling asset: ${SOURCE}"
			VERBATIM
		)

		list(APPEND STAMP_FILES ${STAMP_FILE})
	endforeach()

	add_custom_target(${TARGET}
		DEPENDS
			${STAMP_FILES}
	)
endfunction()

# Every file under this directory is handed to the compiler. It skips the ones whose extension it does not recognise
# with a warning, which is how the authoring files living next to the assets – `.blend`, `.spp` – are ignored.
#
# Globbing is normally discouraged in CMake because adding a file does not trigger a regeneration. `CONFIGURE_DEPENDS`
# fixes that: the glob is re-evaluated on every build and the build system is regenerated when the result changes. So
# dropping a new asset into any of these folders is enough, no edit here required.
file(GLOB_RECURSE BLK_ASSET_SOURCES
	CONFIGURE_DEPENDS
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/*
)

# This file is not an asset.
list(REMOVE_ITEM BLK_ASSET_SOURCES CMakeLists.txt)

blk_add_assets(BlkAssets ${BLK_ASSET_SOURCES})
