cmake_minimum_required (VERSION 3.1)
project (obj_renderer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")

find_package(Boost REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/glfw)

set(GLFW_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/dep/glfw/include)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.c")

include_directories(${CMAKE_SOURCE_DIR}/src
                    ${CMAKE_SOURCE_DIR}/include
                    ${BOOST_INCLUDE_DIRS}
                    ${GLFW_INCLUDE_DIRS}
                    ${GLEW_INCLUDE_DIRS}
                    ${GL_INCLUDE_DIRS}
                    ${MATHFU_INCLUDE_DIRS})

add_executable(lcycles ${SOURCE_FILES})
target_link_libraries(lcycles glfw ${GLFW_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})
