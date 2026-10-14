option(TRACY_ENABLE "" ON)
# To record events immediately when the engine launches, disable this option.
option(TRACY_ON_DEMAND "" ON)

add_subdirectory(box3d-0.1.0)
add_subdirectory(IconFontCppHeaders-main)
add_subdirectory(imgui-v1.92.8)
add_subdirectory(stb-v2.30)
add_subdirectory(tracy-0.14.1)
add_subdirectory(ufbx-v0.23.0)


