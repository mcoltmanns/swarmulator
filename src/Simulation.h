//
// Created by moltmanns on 4/13/25.
//

#ifndef SIMULATION_H
#define SIMULATION_H
#include <vector>
#include <filesystem>
#include <fstream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/file.hpp>

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

    int max_agents_ = 5000;
    int min_agents_ = 100;
    int max_objects_ = 100;

    float log_interval_ = 1; // how many seconds of simtime between loggings
    float last_log_time_ = 0;
    double time_ = 0; // time since Simulation() called
    std::filesystem::path log_file_path_;
    boost::iostreams::filtering_ostream log_file_str_;
    std::stringstream log_buffer_;
    size_t log_buffer_max_size_ = 2048 * 2048 * 2048; // how many characters to accumulate between writing log to disk
    // bigger is better, but bigger needs more memory

    std::list<std::shared_ptr<AgentType>> agents_;
    swarmulator::agent::SSBOAgent* agents_ssbo_array_;
    static constexpr size_t agents_ssbo_size_ = 1024 * 1024;
    unsigned int agents_ssbo_;

    std::list<std::shared_ptr<swarmulator::env::Sphere>> objects_;

    swarmulator::util::StaticGrid<AgentType> grid_;

    size_t total_agents_ = 0; // number of agents that have existed over the course of the whole simulation

public:
    bool logging_enabled_ = false;

    Simulation() : grid_(world_size_, grid_divisions_) {
        agents_ssbo_array_ = static_cast<swarmulator::agent::SSBOAgent *>(RL_CALLOC(agents_ssbo_size_, sizeof(swarmulator::agent::SSBOAgent)));
        agents_ssbo_ = rlLoadShaderBuffer(agents_ssbo_size_ * sizeof(swarmulator::agent::SSBOAgent), agents_ssbo_array_, RL_DYNAMIC_COPY);
    }

    Simulation(Vector3 world_size, int grid_divisions) :
    world_size_(world_size), grid_divisions_(grid_divisions), grid_(world_size, grid_divisions) {
        agents_ssbo_array_ = static_cast<swarmulator::agent::SSBOAgent *>(RL_CALLOC(agents_ssbo_size_, sizeof(swarmulator::agent::SSBOAgent)));
        agents_ssbo_ = rlLoadShaderBuffer(agents_ssbo_size_ * sizeof(swarmulator::agent::SSBOAgent), agents_ssbo_array_, RL_DYNAMIC_COPY);
    }

    ~Simulation() {
        RL_FREE(agents_ssbo_array_);
    }

    [[nodiscard]] bool can_add_agent() const { return agents_.size() < max_agents_; }
    [[nodiscard]] bool can_add_object() const { return objects_.size() < max_objects_; }

    [[nodiscard]] const std::vector<std::shared_ptr<swarmulator::agent::Agent>> &get_agents() const { return agents_; }
    [[nodiscard]] size_t get_agents_count() const { return agents_.size(); }
    [[nodiscard]] size_t get_max_agents() const { return max_agents_; }
    [[nodiscard]] size_t get_total_agents() const { return total_agents_; }
    void set_min_agents(const size_t min_agents) { min_agents_ = min_agents; }
    void set_max_agents(const size_t max_agents) { max_agents_ = max_agents; }

    [[nodiscard]] const std::list<std::shared_ptr<swarmulator::env::Sphere>> &get_objects() const { return objects_; }
    [[nodiscard]] size_t get_objects_count() const { return objects_.size(); }

    [[nodiscard]] double sim_time() const { return time_; }

    void set_log_file(const std::filesystem::path &dir) {
        log_file_path_ = dir;
        std::cout << "logging to " << log_file_path_ << std::endl;
        //const auto src = boost::iostreams::file_source(log_file_path_.c_str(), std::ios::out | std::ios::trunc);
        auto file = std::ofstream(log_file_path_, std::ios::out | std::ios::trunc | std::ios::binary);
        log_file_str_.reset();
        log_file_str_.push(boost::iostreams::gzip_compressor());
        log_file_str_.push(boost::iostreams::file_sink(log_file_path_));
        log_file_str_ << "time | id | genome | position | rotation | signals | info | parent" << std::endl;
    }

    // should be threadsafe?
    void add_agent(const std::shared_ptr<AgentType> &agent) {
        agents_.emplace_back(agent);
#pragma omp atomic update
        ++total_agents_;
    }

    // not threadsafe
    void add_object(const std::shared_ptr<swarmulator::env::Sphere> &object) {
        objects_.emplace_back(object);
    }

    void update(float dt) {
        time_ += dt;
        // remove dead agents and wrap bounds
        auto it = agents_.begin();
        while (it != agents_.end()) {
            if (!(*it)->is_alive()) {
                it = agents_.erase(it);
            }
            else {
                (*it)->set_position(grid_.wrap_position((*it)->get_position()));
                //grid_.bounce_agent(*it);
                ++it;
            }
        }
        if (agents_.size() < min_agents_) {
            for (auto i = 0; i < min_agents_ - agents_.size(); i++) {
                auto new_agent = std::make_shared<AgentType>();
                const auto p = Vector4{(randfloat() - 0.5f) * world_size_.x, (randfloat() - 0.5f) * world_size_.y, (randfloat() - 0.5f) * world_size_.z, 0};
                const auto r = Vector4{randfloat() - 0.5f, randfloat() - 0.5f, randfloat() - 0.5f, 0};
                new_agent->set_position(xyz(p));
                new_agent->set_direction(xyz(r));
                new_agent->set_time_born(time_);
                add_agent(new_agent);
            }
        }
        swarmulator::agent::global_reward_factor = std::lerp(1.f, 0.f, static_cast<float>(static_cast<int>(agents_.size()) - min_agents_) / static_cast<float>(max_agents_ - min_agents_)); // keeps population in acceptable range
        //swarmulator::agent::global_reward_factor = static_cast<float>(max_agents_ - 1) / static_cast<float>(std::max(static_cast<int>(agents_.size()) - min_agents_, 1)) - 1.f; // tune the GRF inverse exponentially - between 0 and max_agents_
        // throw agents into the grid
        grid_.sort_agents(agents_);
        int buffer_write_place = 0;

        std::string time_str = std::to_string(time_);
        bool are_we_logging = time_ - last_log_time_ >= log_interval_;

        // iteration is cheap, processing is expensive
        // so have every thread iterate over the whole list
        // but only a single thread will access a specific element (single) while the others continue on (nowait)
        // this has no noticeable performance impact vs parallel vector access
        // but this is still the most expensive step (about 2x slower than grid sorting)
#pragma omp parallel private(it)
        {
            // update live agents
            for (it = agents_.begin(); it != agents_.end(); ++it) {
#pragma omp single nowait
                {
                    const auto agent = *it;
                    auto neighborhood = grid_.get_neighborhood(agent.operator*());
                    auto new_agent = std::dynamic_pointer_cast<AgentType>(agent->update(neighborhood.operator*(), objects_, dt));
                    // critical section wastes little time - several orders of magnitude less than updates
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
                    if (are_we_logging && logging_enabled_) {
                        last_log_time_ = time_;
                        // always: time, id, genome, pos, rot, sig a, sig b, info x, info y, parent id
                        std::string id_str = boost::uuids::to_string(agent->get_id());
                        // put together the genome - array of all weights and biases
                        std::string genome_str = agent->get_genome_string();
                        std::string parent_str = boost::uuids::to_string(agent->get_parent());
                        swarmulator::agent::SSBOAgent into;
                        agent->to_ssbo(&into);
#pragma omp critical
                        log_buffer_ <<
                            time_str << " | " <<
                            id_str << " | " <<
                            genome_str << " | " <<
                            Vector3ToString(xyz(into.position)) << " | " <<
                            Vector3ToString(xyz(into.direction)) << " | " <<
                            Vector2ToString(into.signals) << " | " <<
                            Vector2ToString(into.info) << " | " <<
                            parent_str << std::endl;
                        // concurrent stream access big no-no!
                    }
                }
            }
        }
        rlUpdateShaderBuffer(agents_ssbo_, agents_ssbo_array_, agents_.size() * sizeof(swarmulator::agent::SSBOAgent), 0); // only copy as much data as there are agents (actually saves a lot of time)
        if (log_buffer_.tellp() >= log_buffer_max_size_) {
            log_file_str_ << log_buffer_.str();
            log_buffer_ = std::stringstream(); // get a whole new stringstream (this also resets the write position)
        }
    }

    [[nodiscard]] unsigned int get_agents_ssbo() const { return agents_ssbo_; }

    void draw_objects() const {
        for (const auto &object : objects_) {
            object->draw();
            object->setColor(BLUE);
        }
    }
};

#endif //SIMULATION_H
