cmake_minimum_required(VERSION 3.9)
project(swarmulator_cpp)

set(CMAKE_CXX_STANDARD 20)

# EXTERNAL HEADERS
set(INCLUDE_DIR "include")
include_directories(${INCLUDE_DIR})

# SOURCES
set(PROJECT_SOURCE_DIR "src")

# BOIDS SOURCES
set(BOIDS_SOURCES "${PROJECT_SOURCE_DIR}/boids.cpp")

# RAYLIB
set(RAYLIB_VERSION 5.5)
find_package(raylib ${RAYLIB_VERSION} QUIET)
if (NOT raylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(
            raylib
            DOWNLOAD_EXTRACT_TIMESTAMP OFF
            URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    )
    FetchContent_GetProperties(raylib)
    if (NOT raylib_POPULATED)
        set(FETCHCONTENT_QUIET NO)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(GRAPHICS GRAPHICS_API_OPENGL_43) # need ogl4.3 for compute shaders
        FetchContent_MakeAvailable(raylib)
    endif ()
endif ()
# platform-specific raylib config
# Web
if (${PLATFORM} STREQUAL "Web")
    set_target_properties(boids PROPERTIES SUFFIX ".html")
    set_target_properties(smartboids PROPERTIES SUFFIX ".html")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1")
endif ()
# Apple
if (APPLE)
    target_link_libraries(boids "-framework IOKit")
    target_link_libraries(boids "-framework Cocoa")
    target_link_libraries(boids "-framework OpenGL")
    target_link_libraries(smartboids "-framework IOKit")
    target_link_libraries(smartboids "-framework Cocoa")
    target_link_libraries(smartboids "-framework OpenGL")
endif ()

add_executable(boids ${BOIDS_SOURCES})
target_link_libraries(boids raylib)
