cmake_minimum_required(VERSION 3.9)
project(swarmulator_cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# EXTERNAL HEADERS
set(EXTERNAL_DIR "external")
include_directories(${EXTERNAL_DIR})

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
#[[
if (${PLATFORM} STREQUAL "Web")
    set_target_properties(swarmulator PROPERTIES SUFFIX ".html")
    set_target_properties(swarmulator PROPERTIES SUFFIX ".html")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1")
endif ()
# Apple
if (APPLE)
    target_link_libraries(swarmulator "-framework IOKit")
    target_link_libraries(swarmulator "-framework Cocoa")
    target_link_libraries(swarmulator "-framework OpenGL")
endif ()
]]

# OPENMP (for cpu-side multithreading)
# available through package managers
find_package(OpenMP REQUIRED)

# BOOST (we actually don't use this anymore?)
#find_package(Boost REQUIRED COMPONENTS iostreams)
#if (Boost_FOUND)
#    add_definitions(${BOOST_DEFINITIONS} -DBOOST_ALL_NO_LIB)
#    include_directories(${Boost_INCLUDE_DIRS})
#    link_directories(${Boost_LIBRARY_DIRS})
#endif ()

find_package(HDF5 COMPONENTS CXX HL REQUIRED)

set(AGENTS_SOURCES
    src/agent/Boid.cpp
    src/agent/Boid.h
)

set(SIM_SOURCES
        src/sim/SimObject.h
        src/sim/SimObject.cpp
        src/sim/StaticGrid.h
        src/sim/StaticGrid.cpp
        src/sim/Simulation.h
        src/sim/Simulation.cpp
        src/sim/util.h
        src/sim/ObjectInstancer.cpp
        src/sim/ObjectInstancer.h
        src/sim/Logger.cpp
        src/sim/Logger.h
)

set(SHADER_SOURCES
        src/shaders/simobject.frag
        src/shaders/boid.vert
)

add_executable(swarmulator_boids_grid
        src/boids_grid.cpp
        external/raygui.h
        ${AGENTS_SOURCES}
        ${SHADER_SOURCES}
        ${SIM_SOURCES}
)
target_link_libraries(swarmulator_boids_grid raylib OpenMP::OpenMP_CXX HDF5::HDF5) #${Boost_LIBRARIES})
