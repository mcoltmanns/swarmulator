//
// Created by moltmanns on 4/13/25.
//

#ifndef SIMULATION_H
#define SIMULATION_H
#include <vector>

#include "agent/Agent.h"
#include "env/Sphere.h"
#include "util/StaticGrid.h"

// this is the upper limit on simulation size
// there does need to be a hard limit because gpu buffer sizes are fixed and expensive to reallocate
#define MAX_AGENTS 1024 * 1024

class Simulation {
private:
    Vector3 world_size_ = {1.f, 1.f, 1.f};
    int grid_divisions_ = 20;
    float time_scale_ = 1.f;

    std::vector<swarmulator::agent::Agent *> agents_; // this must be a vector! else parallelization is hard
    std::vector<swarmulator::env::Sphere *> objects_;
    swarmulator::util::StaticGrid grid_;

    // arrays for the gpu buffers
    Vector4 *agent_positions_ = nullptr;
    Vector4 *agent_directions_ = nullptr;

    // actual gpu buffers
    unsigned int agent_position_ssbo_ = -1;
    unsigned int agent_direction_ssbo_ = -1;

public:
    ~Simulation() = default;

    Simulation();
    Simulation(Vector3 world_size, int grid_divisions);
    unsigned long add_agent(swarmulator::agent::Agent *agent);
    //swarmulator::agent::Agent *get_agent(const int agent_id) const { return agents_[agent_id]; }
    //void remove_agent(const std::_List_iterator<swarmulator::agent::Agent *> agent_it) { agents_.erase(agent_it); }

    int add_object(swarmulator::env::Sphere *object);
    swarmulator::env::Sphere *get_object(const int object_id) const { return objects_[object_id]; }
    void remove_object(const int object_id) { objects_.erase(objects_.begin() + object_id); };

    void update(float dt);
};



#endif //SIMULATION_H
