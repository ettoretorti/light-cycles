cmake_minimum_required (VERSION 3.1)
project (lcycles)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/glfw)

set(GLFW_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/dep/glfw/include)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.c")
#source groups split files into the correct hierarchy in VS
source_group(src REGULAR_EXPRESSION ${CMAKE_SOURCE_DIR}/src/*)
source_group(src\\gl REGULAR_EXPRESSION ${CMAKE_SOURCE_DIR}/src/gl/*)
source_group(src\\gfx REGULAR_EXPRESSION ${CMAKE_SOURCE_DIR}/src/gfx/*)
source_group(src\\lcycle REGULAR_EXPRESSION ${CMAKE_SOURCE_DIR}/src/lcycle/*)

add_executable(lcycles ${SOURCE_FILES})

target_include_directories(lcycles
                           PRIVATE ${CMAKE_SOURCE_DIR}/src
                           PRIVATE ${CMAKE_SOURCE_DIR}/include
                           PRIVATE ${GLFW_INCLUDE_DIRS}
                           PRIVATE ${GL_INCLUDE_DIRS})

target_compile_options(lcycles
                       PRIVATE -Wall)

if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    #g++ options
    target_compile_options(lcycles
                           PRIVATE -Wextra
                           PRIVATE -pedantic)
endif()

target_link_libraries(lcycles
                      glfw
                      ${GLFW_LIBRARIES}
                      ${OPENGL_LIBRARIES})
