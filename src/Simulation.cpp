//
// Created by moltmanns on 4/13/25.
//

#include "Simulation.h"

#include "rlgl.h"
#include "raymath.h"

Simulation::Simulation() : grid_(world_size_, grid_divisions_) {
    agent_positions_ = static_cast<Vector4 *>(RL_CALLOC(MAX_AGENTS, sizeof(Vector4)));
    agent_directions_ = static_cast<Vector4 *>(RL_CALLOC(MAX_AGENTS, sizeof(Vector4)));
    agent_position_ssbo_ = rlLoadShaderBuffer(0, agent_positions_, RL_DYNAMIC_COPY);
    agent_direction_ssbo_ = rlLoadShaderBuffer(0, agent_directions_, RL_DYNAMIC_COPY);
}

Simulation::Simulation(const Vector3 world_size, const int grid_divisions) :
    world_size_(world_size), grid_divisions_(grid_divisions), grid_(world_size, grid_divisions) {}

unsigned long Simulation::add_agent(swarmulator::agent::Agent *agent) {
    agents_.push_back(agent);
    return agents_.size() - 1;
}

int Simulation::add_object(swarmulator::env::Sphere *object) {
    objects_.push_back(object);
    return objects_.size() - 1;
}

void Simulation::update(const float dt) {
    grid_.sort_agents(agents_);
}
