cmake_minimum_required(VERSION 3.23)

project(Blink VERSION 0.0.1 LANGUAGES CXX C)

set(BLK_RUNTIME_DIRECTORY ${CMAKE_BINARY_DIR}/Runtime/$<CONFIG>)

add_subdirectory(External)
add_subdirectory(Engine)
add_subdirectory(Compiler)
add_subdirectory(Assets)
add_subdirectory(Game)
add_subdirectory(Launcher)
