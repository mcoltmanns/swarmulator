cmake_minimum_required(VERSION 3.9)
project(swarmulator_cpp)

set(CMAKE_CXX_STANDARD 20)

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

# OPENMP
find_package(OpenMP REQUIRED)

# FANN
# need fann-dev and fann 2
# shouldn't need fann any longer
#find_package(fann)

# EIGEN
find_package(Eigen3 REQUIRED)

# BOOST
find_package(Boost REQUIRED COMPONENTS iostreams)
if (Boost_FOUND)
    add_definitions(${BOOST_DEFINITIONS} -DBOOST_ALL_NO_LIB)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
endif ()


set(AGENTS_SOURCES
    src/agent/Agent.cpp
    src/agent/Agent.h
    external/v3ops.h
    external/raygui.h
    src/agent/NeuralAgent.cpp
    src/agent/NeuralAgent.h
    src/agent/ForageAgent.cpp
    src/agent/ForageAgent.h
    src/agent/Boid.cpp
    src/agent/Boid.h
    src/agent/PDAgent.cpp
    src/agent/PDAgent.h
    src/agent/PredatorAgent.cpp
    src/agent/PredatorAgent.h
    src/agent/PreyAgent.cpp
    src/agent/PreyAgent.h
)

set(ENV_SOURCES
    src/env/Sphere.cpp
    src/env/Sphere.h
)

set(LOGGER_SOURCES
)

set(UTIL_SOURCES
        src/util/StaticGrid.h
        src/util/util.h
)

set(SHADER_SOURCES
        src/shaders/agent.frag
        src/shaders/boid.vert
        src/shaders/forage.vert
        src/shaders/pd.vert
)

# forage experiment
add_executable(swarmulator_forage
        src/continuous_forage.cpp
        src/Simulation.h
        ${AGENTS_SOURCES}
        ${ENV_SOURCES}
        ${LOGGER_SOURCES}
        ${UTIL_SOURCES}
        ${SHADER_SOURCES}
)
target_link_libraries(swarmulator_forage Eigen3::Eigen raylib OpenMP::OpenMP_CXX ${Boost_LIBRARIES})

# prisoner's dilemma experiment
add_executable(swarmulator_pd
        src/prisoners_dilemma.cpp
        src/Simulation.h
        ${AGENTS_SOURCES}
        ${ENV_SOURCES}
        ${LOGGER_SOURCES}
        ${SHADER_SOURCES}
        ${UTIL_SOURCES}
)
target_link_libraries(swarmulator_pd Eigen3::Eigen raylib OpenMP::OpenMP_CXX ${Boost_LIBRARIES})

# predator/prey experiment
add_executable(swarmulator_pred_prey
        src/pred_prey.cpp
        src/Simulation.h
        ${AGENTS_SOURCES}
        ${ENV_SOURCES}
        ${LOGGER_SOURCES}
        ${SHADER_SOURCES}
        ${UTIL_SOURCES}
)
target_link_libraries(swarmulator_pred_prey Eigen3::Eigen raylib OpenMP::OpenMP_CXX ${Boost_LIBRARIES})

# boids no grid
add_executable(swarmulator_boids_nogrid
        src/boids_nogrid.cpp
        src/Simulation.h
        ${AGENTS_SOURCES}
        ${ENV_SOURCES}
        ${LOGGER_SOURCES}
        ${UTIL_SOURCES}
        ${SHADER_SOURCES}
)
target_link_libraries(swarmulator_boids_nogrid Eigen3::Eigen raylib OpenMP::OpenMP_CXX ${Boost_LIBRARIES})

add_executable(swarmulator_boids_grid
        src/boids_grid.cpp
        src/Simulation.h
        ${AGENTS_SOURCES}
        ${ENV_SOURCES}
        ${LOGGER_SOURCES}
        ${UTIL_SOURCES}
        ${SHADER_SOURCES}
)
target_link_libraries(swarmulator_boids_grid Eigen3::Eigen raylib OpenMP::OpenMP_CXX ${Boost_LIBRARIES})
