cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(swarmulator_cpp)

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

# HDF5 (for logging)
# available through package managers
find_package(HDF5 COMPONENTS CXX REQUIRED)

find_package(Eigen3 REQUIRED)

set(AGENTS_SOURCES
        src/objects/Boid.cpp
        src/objects/Boid.h
        src/objects/NeuralAgent.cpp
        src/objects/NeuralAgent.h
        src/objects/ForageSource.h
        src/objects/PDAgent.cpp
        src/objects/PDAgent.h
        src/objects/ForageAgent.cpp
        src/objects/ForageAgent.h
        src/objects/PDWatcher.h
)

set(LOGGER_SOURCES
        src/sim/logger/Logger.cpp
        src/sim/logger/Logger.h
        src/sim/logger/ThreadsafeQueue.h
        src/sim/logger/LogTask.h
)

set(SIM_SOURCES
        external/raygui.h
        src/sim/SimObject.h
        src/sim/SimObject.cpp
        src/sim/StaticGrid.h
        src/sim/StaticGrid.cpp
        src/sim/Simulation.h
        src/sim/Simulation.cpp
        src/sim/util.h
        src/sim/ObjectInstancer.cpp
        src/sim/ObjectInstancer.h
)

add_executable(swarmulator_boids_grid
        src/boids_grid.cpp
        ${AGENTS_SOURCES}
        ${SIM_SOURCES}
        ${LOGGER_SOURCES}
)
target_link_libraries(swarmulator_boids_grid raylib OpenMP::OpenMP_CXX HDF5::HDF5 Eigen3::Eigen)

add_executable(swarmulator_forage
        src/forage.cpp
        ${AGENTS_SOURCES}
        ${SIM_SOURCES}
        ${LOGGER_SOURCES}
)
target_link_libraries(swarmulator_forage raylib OpenMP::OpenMP_CXX HDF5::HDF5 Eigen3::Eigen)

add_executable(swarmulator_pd
        src/pd.cpp
        ${AGENTS_SOURCES}
        ${SIM_SOURCES}
        ${LOGGER_SOURCES}
)
target_link_libraries(swarmulator_pd raylib OpenMP::OpenMP_CXX HDF5::HDF5 Eigen3::Eigen)
