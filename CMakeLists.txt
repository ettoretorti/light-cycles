cmake_minimum_required (VERSION 3.1)
project (obj_renderer)

set(CMAKE_CXX_STANDARD 11)

find_package(Boost REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/glfw)

set(GLFW_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/dep/glfw/include)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.c")

add_executable(lcycles ${SOURCE_FILES})

target_include_directories(lcycles
	                   PRIVATE ${CMAKE_SOURCE_DIR}/src
                           PRIVATE ${CMAKE_SOURCE_DIR}/include
                           PRIVATE ${BOOST_INCLUDE_DIRS}
                           PRIVATE ${GLFW_INCLUDE_DIRS}
                           PRIVATE ${GLEW_INCLUDE_DIRS}
                           PRIVATE ${GL_INCLUDE_DIRS}
                           PRIVATE ${MATHFU_INCLUDE_DIRS})

target_compile_options(lcycles
	               PRIVATE -Wall
		       PRIVATE -Wextra
		       PRIVATE -pedantic)

target_link_libraries(lcycles
                      glfw
                      ${GLFW_LIBRARIES}
                      ${GLEW_LIBRARIES}
                      ${OPENGL_LIBRARIES})
