cmake_minimum_required(VERSION 2.8)

project(graf)

# Includes directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../light/include)
include_directories(${CMAKE_SOURCE_DIR}/../red/include)

# Lib: OpenGL
find_package(OpenGL REQUIRED)
set(LIBS ${LIBS} ${OPENGL_LIBRARIES})

# Lib: light
set(LIBS ${LIBS} "${CMAKE_SOURCE_DIR}/../light/build/liblight.a")

# Lib: Linux specific
if(UNIX)
	# Lib: dl
	set(LIBS ${LIBS} "dl")

	# Lib: rt
	set(LIBS ${LIBS} "rt")
endif(UNIX)

# Files belonging to the project
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")
file(GLOB_RECURSE SOURCE_FILES ${SOURCE_FILES} "src/gl3w/*.c")
file(GLOB_RECURSE SOURCE_FILES ${SOURCE_FILES} "test/*.cpp")
file(GLOB_RECURSE HEADER_FILES "include/*.hpp")
file(GLOB_RECURSE HEADER_FILES ${HEADER_FILES} "test/*.hpp")

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall")

# Debug mode
if(CMAKE_BUILD_TYPE MATCHES Debug)
	add_definitions(-D_DEBUG)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

add_executable(graf ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(graf ${LIBS})
