//
// Created by moltma on 11/20/25.
//

#include <iostream>
#include <omp.h>

#include "raylib.h"
#include "sim/Simulation.h"
#include "sim/util.h"

int main(int argc, char** argv) {
    int init_agent_count = 200;
    int window_w = 1080;
    int window_h = 720;
    constexpr Vector3 world_size = {100, 100, 100};
    constexpr int subdivisions = 20;
    std::string log_path = "./pred_prey.h5";

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
    if (const auto o = swarmulator::get_opt(argv, argv + argc, "--logfile")) {
        log_path = o;
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

    auto simulation = swarmulator::Simulation(window_w, window_h, world_size, subdivisions, 0.01, 100000000, log_path, 4, 20); // 100 million (1e8) updates at 0.1 dt each is ten million (1e7) simulation time
    // 10 million updates (1e7) at 1 dt each is 10 million simulation time
    // at a log interval of 20 we get 500k log entries with a 20 time gap between each
    //auto simulation = swarmulator::Simulation(window_w, window_h, world_size, subdivisions);

    /*
     * pred-prey simulation is supposed to be a chance for more complexity in the system
     * pd only has two ways for agents to interact (cooperate or defect)
     * this introduces more
     * three types of agents: plant, prey, predator
     *  plants
     *      plants have a small interaction radius, do not move, are not "smart", and have their population kept constant by the simulation
     *      plants do not lose energy by existing and are born with 10 energy each
     *      prey gain energy when spending time in a plant's interaction radius (this saps energy from the plant)
     *  prey 
     *      prey are smart, mobile, and reproduce when they gain enough energy from foraging plants
     *      they have a base living cost, a signalling cost, and lose energy when attacked by predators
     *      their population is kept at a minimum by the simulation
     *  predators
     *      predators are smart, mobile, and reproduce when they gain enough energy from eating prey or each other
     *      they have a base living cost, a signalling cost, and lose energy when attacked by predators
     *      their population is kept at a minimum by the simulation
     *
     * things to consider:
     * - should prey be faster than predators?
     * - should predators be able to move faster at an increased energy cost?
     *
     * - what should the ratios be?
     *      according to https://pmc.ncbi.nlm.nih.gov/articles/PMC3061189/
     *      carnivore populations are negatively affected by body mass, positively affected by prey availability
     *      ratio of predator:prey never exceeds 1:1, drops as low as 0.2:1
     *
     *      maybe start out with a 1:2:3 ratio of predator:prey:plant
     *
     * - should predators eat everything they see? (probably not, see Nicholson, A. J. (1933). The balance of animal populations. J. Animal Ecol. 2(Suppl. 1), 132–178.)
     *      - maybe decrease the chance of a predator attacking based on how far from death it is
     *      - or bias predator attacks in favor of young/weak prey (Genovart, M.; Negre, N.; Tavecchia, G.; Bistuer, A.; Parpal, L.; Oro, D. (2010). "The young, the weak and the sick: evidence of natural selection by predation")
     *
     * - see if population counts follow the lotka-volterra model (Volterra, V. (1931). "Variations and fluctuations of the number of individuals in animal species living together". In Chapman, R. N. (ed.). Animal Ecology. McGraw–Hill.) (Lotka, A. J. (1925). Elements of Physical Biology. Williams and Wilkins.)
     *      - important corollary of lotka-volterra - increase in prey equilibrium density makes environment more favorable to predator, not to prey
     *      keep that in mind when adjusting parameters!
     */

    // fragment shader and triangle mesh for all objects
    const std::string fs_src_path = "/home/moltma/Documents/swarmulator/src/shaders/simobject.frag";
    const auto tri = std::vector<Vector3>{
                    { -0.86, -0.5, 0.0 },
                    { 0.86, -0.5, 0.0 },
                    { 0.0f,  1.0f, 0.0f }
    };
}

