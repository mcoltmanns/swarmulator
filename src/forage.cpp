#include <iostream>
#include <omp.h>


#include "objects/ForageAgent.h"
#include "objects/ForageSource.h"
#include "raylib.h"
#include "sim/Simulation.h"
#include "sim/util.h"
//
// Created by moltma on 10/23/25.
//
int main(int argc, char** argv) {
    int init_agent_count = 200;
    int food_source_count = 1;
    int window_w = 1080;
    int window_h = 720;
    constexpr Vector3 world_size = {100, 100, 100};
    constexpr int subdivisions = 20;

    if (const auto o = swarmulator::get_opt(argv, argv + argc, "-n")) {
        init_agent_count = std::stoi(o);
    }
    if (const auto o = swarmulator::get_opt(argv, argv + argc, "-w")) {
        window_w = std::stoi(o);
    }
    if (const auto o = swarmulator::get_opt(argv, argv + argc, "-h")) {
        window_h = std::stoi(o);
    }
    if (const auto o = swarmulator::get_opt(argv, argv + argc, "-t")) {
        omp_set_num_threads(std::stoi(o));
    }
    if (const auto o = swarmulator::get_opt(argv, argv + argc, "-p")) {
        omp_set_num_threads(std::max(1, std::min(std::stoi(o), omp_get_max_threads()))); // number of threads used should not be greater than the maximum threads available on device
    }
    if (swarmulator::opt_exists(argv, argv + argc, "--vsync")) {
        SetConfigFlags(FLAG_VSYNC_HINT);
    }
    if (swarmulator::opt_exists(argv, argv + argc, "--aa")) {
        SetConfigFlags(FLAG_MSAA_4X_HINT);
    }

    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN); // keep running even when focus lost

    // seed rng
    auto s = time(nullptr);
    srand(s);
    std::cout << "Random seed: " << s << std::endl;

    auto simulation = swarmulator::Simulation(window_w, window_h, world_size, subdivisions, 1, 1000, "forage_small.h5", 4, 1); // 100 million (1e8) updates at 0.1 dt each is ten million (1e7) simulation time
    // 10 million updates (1e7) at 1 dt each is 10 million simulation time
    // at a log interval of 20 we get 500k log entries with a 20 time gap between each

    // add the foragers
    std::string vs_src_path = "/home/moltma/Documents/swarmulator/src/shaders/neural.vert";
    const std::string fs_src_path = "/home/moltma/Documents/swarmulator/src/shaders/simobject.frag";
    const auto tri = std::vector<Vector3>{
                { -0.86, -0.5, 0.0 },
                { 0.86, -0.5, 0.0 },
                { 0.0f,  1.0f, 0.0f }
    };
    simulation.new_object_type<swarmulator::ForageAgent>(tri, vs_src_path, fs_src_path);

    // add the food
    vs_src_path = "/home/moltma/Documents/swarmulator/src/shaders/blue.vert";
    simulation.new_object_type<swarmulator::ForageSource>(tri, vs_src_path, fs_src_path);

    // initialize the foragers
    for (int i = 0; i < init_agent_count; ++i) {
        const auto p = Vector3 {
            swarmulator::randfloat(-world_size.x / 2.f, world_size.x / 2.f),
            swarmulator::randfloat(-world_size.y / 2.f, world_size.y / 2.f),
            swarmulator::randfloat(-world_size.z / 2.f, world_size.z / 2.f)
        };
        const auto r = Vector3 {
            swarmulator::randfloat(), swarmulator::randfloat(), swarmulator::randfloat()
        };
        auto obj = swarmulator::ForageAgent(p, r);
        simulation.add_object(obj);
    }

    // initialize the food sources
    for (int i = 0; i < food_source_count; ++i) {
        const auto p = Vector3 {
            swarmulator::randfloat(-world_size.x / 4.f, world_size.x / 4.f),
            swarmulator::randfloat(-world_size.y / 4.f, world_size.y / 4.f),
            swarmulator::randfloat(-world_size.z / 4.f, world_size.z / 4.f)
        };
        auto obj = swarmulator::ForageSource(p, Vector3UnitX);
        simulation.add_object(obj);
    }

    simulation.run();

    return 0;
}
