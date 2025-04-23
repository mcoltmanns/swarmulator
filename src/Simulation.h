//
// Created by moltmanns on 4/13/25.
//

#ifndef SIMULATION_H
#define SIMULATION_H
#include <vector>

#include "agent/Agent.h"
#include "env/Sphere.h"
#include "util/StaticGrid.h"
#include "util/util.h"

template <class AgentType>
class Simulation {
static_assert(std::is_base_of_v<swarmulator::agent::Agent, AgentType>, "simulations must be run with a class derived from swarmulator::agent::Agent");
private:
    Vector3 world_size_ = {1.f, 1.f, 1.f};
    int grid_divisions_ = 20;
    float time_scale_ = 1.f;

    static constexpr size_t max_agents = 25000;
    static constexpr size_t max_objects = 500;

    std::list<std::shared_ptr<AgentType>> agents_;
    swarmulator::agent::SSBOAgent* agents_ssbo_array_;
    unsigned int agents_ssbo_;

    std::vector<std::shared_ptr<swarmulator::env::Sphere>> objects_;
    size_t current_objects_ = 0;
    size_t next_object_insert = 0;
    swarmulator::util::StaticGrid<AgentType> grid_;

    bool logging_enabled_ = false;

public:
    Simulation() : grid_(world_size_, grid_divisions_) {
        objects_.resize(max_objects);
        for (int i = 0; i < max_objects; ++i) {
            objects_[i] = nullptr;
        }
        agents_ssbo_array_ = static_cast<swarmulator::agent::SSBOAgent *>(RL_CALLOC(max_agents, sizeof(swarmulator::agent::SSBOAgent)));
        agents_ssbo_ = rlLoadShaderBuffer(max_agents * sizeof(swarmulator::agent::SSBOAgent), agents_ssbo_array_, RL_DYNAMIC_COPY);
    }

    Simulation(Vector3 world_size, int grid_divisions) :
    world_size_(world_size), grid_divisions_(grid_divisions), grid_(world_size, grid_divisions) {
        objects_.resize(max_objects);
        for (int i = 0; i < max_objects; ++i) {
            objects_[i] = nullptr;
        }
        agents_ssbo_array_ = static_cast<swarmulator::agent::SSBOAgent *>(RL_CALLOC(max_agents, sizeof(swarmulator::agent::SSBOAgent)));
        agents_ssbo_ = rlLoadShaderBuffer(max_agents * sizeof(swarmulator::agent::SSBOAgent), agents_ssbo_array_, RL_DYNAMIC_COPY);
    }

    ~Simulation() {
        RL_FREE(agents_ssbo_array_);
    }

    [[nodiscard]] bool can_add_agent() const { return agents_.size() < max_agents; }
    [[nodiscard]] bool can_add_object() const { return current_objects_ < max_objects; }

    [[nodiscard]] const std::vector<std::shared_ptr<swarmulator::agent::Agent>> &get_agents() const { return agents_; }
    [[nodiscard]] size_t get_agents_count() const { return agents_.size(); }

    [[nodiscard]] const std::vector<std::shared_ptr<swarmulator::env::Sphere>> &get_objects() const { return objects_; }
    [[nodiscard]] size_t get_objects_count() const { return current_objects_; }

    // should be threadsafe?
    void add_agent(const std::shared_ptr<AgentType> &agent) {
        agents_.emplace_back(agent);
    }

    // not threadsafe
    size_t add_object(const std::shared_ptr<swarmulator::env::Sphere> &object) {
        objects_[next_object_insert] = object;
        ++current_objects_;
        const auto place = next_object_insert;
        for (int i = 0; i < objects_.size(); ++i) {
            if (objects_[i] == nullptr) {
                next_object_insert = i;
                break;
            }
        }
        return place;
    }

    void update(float dt) {
        typename std::list<std::shared_ptr<AgentType>>::iterator it = agents_.begin();
        while (it != agents_.end()) {
            if (!(*it)->is_alive()) {
                it = agents_.erase(it);
            }
            else {
                ++it;
            }
        }
        // throw agents into the grid
        grid_.sort_agents(agents_); // this is not the problem!
        int buffer_write_place = 0;
#pragma omp parallel private(it)
        {
            // update live agents
            for (it = agents_.begin(); it != agents_.end(); ++it) {
#pragma omp single nowait
                {
                    const auto agent = *it;
                    if (agent->get_position().x <= -world_size_.x / 2. || agent->get_position().x >= world_size_.x / 2.
                        || agent->get_position().y <= -world_size_.y / 2. || agent->get_position().y >= world_size_.y / 2.
                        || agent->get_position().z <= -world_size_.z / 2. || agent->get_position().z >= world_size_.z / 2.) {
                        agent->set_direction(agent->get_direction() - 0.5f * agent->get_position());
                        }
                    auto neighborhood = grid_.get_neighborhood(agent.operator*());
                    auto new_agent = std::dynamic_pointer_cast<AgentType>(agent->update(neighborhood.operator*(), objects_, dt));
#pragma omp critical
                    {
                        agent->to_ssbo(&agents_ssbo_array_[buffer_write_place]);
                        ++buffer_write_place;
                        if (new_agent != nullptr && can_add_agent()) {
                            add_agent(new_agent);
                            const auto p = Vector4{(randfloat() - 0.5f) * world_size_.x, (randfloat() - 0.5f) * world_size_.y, (randfloat() - 0.5f) * world_size_.z, 0};
                            const auto r = Vector4{randfloat() - 0.5f, randfloat() - 0.5f, randfloat() - 0.5f, 0};
                            new_agent->set_position(xyz(p));
                            new_agent->set_direction(xyz(r));
                            new_agent->to_ssbo(&agents_ssbo_array_[buffer_write_place]);
                            ++buffer_write_place;
                        }
                    }
                }
            }
        }
        rlUpdateShaderBuffer(agents_ssbo_, agents_ssbo_array_, max_agents * sizeof(swarmulator::agent::SSBOAgent), 0);
    }

    [[nodiscard]] unsigned int get_agents_ssbo() const { return agents_ssbo_; }

    void draw_objects() const {
        for (const auto &object : objects_) {
            if (object == nullptr) {
                continue;
            }
            object->draw();
        }
    }
};

#endif //SIMULATION_H
