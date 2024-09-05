cmake_minimum_required (VERSION 3.5)

project(mydemo)

set(CMAKE_CXX_STANDARD 17)
set(MAIN_PATH ${PROJECT_SOURCE_DIR})

configure_file(include/get_path.h.in include/get_path.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
include_directories(include)

add_subdirectory(src)
add_subdirectory(test)



