//
// Created by moltmanns on 5/14/25.
//

#define RAYGUI_IMPLEMENTATION

#include <omp.h>
#include <string>
#include <H5Cpp.h>

#include "raygui.h"
#include "raylib.h"
#include "agent/Boid.h"
#include "sim/Simulation.h"
#include "sim/util.h"

int main(int argc, char** argv) {
    int init_agent_count = 100;
    int window_w = 1080;
    int window_h = 720;
    constexpr Vector3 world_size = {150, 150, 150};
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

    // seed rng
    auto s = time(nullptr);
    srand(s);
    std::cout << "Random seed: " << s << std::endl;

    auto simulation = swarmulator::Simulation(window_w, window_h, world_size, subdivisions);

    // add the boids
    std::string vs_src_path = "/home/moltma/Documents/swarmulator/src/shaders/boid.vert";
    const std::string fs_src_path = "/home/moltma/Documents/swarmulator/src/shaders/simobject.frag";
    const auto tri = std::vector<Vector3>{
            { -0.86, -0.5, 0.0 },
            { 0.86, -0.5, 0.0 },
            { 0.0f,  1.0f, 0.0f }
    };
    simulation.new_object_type<swarmulator::Boid>(tri, vs_src_path, fs_src_path);

    // add the effectors
    vs_src_path = "/home/moltma/Documents/swarmulator/src/shaders/red.vert";
    simulation.new_object_type<swarmulator::BoidEffector>(tri, vs_src_path, fs_src_path);

    // initialize the boids
    for (int i = 0; i < init_agent_count; i++) {
        const auto p = Vector4{(swarmulator::randfloat() - 0.5f) * world_size.x, (swarmulator::randfloat() - 0.5f) * world_size.y, (swarmulator::randfloat() - 0.5f) * world_size.z, 0};
        const auto r = Vector4{swarmulator::randfloat() - 0.5f, swarmulator::randfloat() - 0.5f,
                               swarmulator::randfloat() - 0.5f, 0};
        auto obj = swarmulator::Boid(swarmulator::xyz(p), swarmulator::xyz(r));
        simulation.add_object(obj);
    }

    // initialize the effectors
    for (int i = 0; i < 50; i++) {
        const auto p = Vector4{(swarmulator::randfloat() - 0.5f) * world_size.x, (swarmulator::randfloat() - 0.5f) * world_size.y, (swarmulator::randfloat() - 0.5f) * world_size.z, 0};
        const auto r = Vector4{swarmulator::randfloat() - 0.5f, swarmulator::randfloat() - 0.5f,
                               swarmulator::randfloat() - 0.5f, 0};
        auto obj = swarmulator::BoidEffector(swarmulator::xyz(p), swarmulator::xyz(r));
        simulation.add_object(obj);
    }

    simulation.run();

    return 0;
}