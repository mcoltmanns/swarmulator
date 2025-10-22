//
// Created by moltmanns on 9/24/25.
//

#ifndef SWARMULATOR_CPP_SIMULATION_H
#define SWARMULATOR_CPP_SIMULATION_H

#include "ObjectInstancer.h"
#include "StaticGrid.h"
#include "logger/Logger.h"

namespace swarmulator {
class Simulation {
protected:
    Vector3 world_size_ = {1, 1, 1};
    int grid_divisions_ = 20;

    // we need a camera
    Camera camera_{};

    // simulation gets a grid
    StaticGrid grid_;
    // simulation gets an object instancer
    ObjectInstancer object_instancer_;
    // and a logger
    Logger logger_;

    // how much simulation time to run for (0 for endless)
    float run_for_ = 0;
    // how much simulation time has passed
    float total_time_ = 0;
    // how much time passes per update (0 for real time)
    float time_step_ = 0;
    // how many updates have been performed (same as number of frames rendered)
    size_t total_steps_ = 0;
    // how many threads the simulation is running on
    int sim_threads_;

    // perform one update
    // dt is the amount of time that has passed since the last update
    // for a realtime simulation, pass GetFrameTime()
    // for a fixed-time simulation, pass a value between 0 and 1
    // log is true if this update should run the logger as well
    void update(float dt, bool log);

    // log static simulation information - parameters which won't change over time
    // this is called once after logger initialization, if the logger was initialized
    virtual std::vector<float> log_static() { return {}; };
    // log dynamic simulation information - simulation instance variables which can change
    // this is called at every log step
    virtual std::vector<float> log_dynamic() { return {}; };

public:
    // sane defaults, no logger, unlimited runtime
    Simulation();

    // specify window and world size, still no logger, unlimited runtime
    Simulation(int win_w, int win_h, Vector3 world_size, int grid_divisions);

    // specify window and world size, as well as logger and compression level
    // total number of log entries must be known for the logger to run
    // TODO write logging mode constructor
    Simulation(int win_w, int win_h, Vector3 world_size, int grid_divisions, std::string& logfile_path, int log_compression, int log_interval_);

    // Simulation(const Vector3 world_size, const int grid_divisions, const std::string& log_path, const size_t
    // max_steps);

    virtual ~Simulation() = default;

    // add a new simobject type to the simulation
    // associate it with a mesh as well as vertex and fragment shaders
    // set up its tables in the logger and log its static data, if the simulation was set up to log
    template<class T>
    void new_object_type(const std::vector<Vector3>& mesh, const std::string& vertex_src_path, const std::string& fragment_src_path) {
        object_instancer_.new_group<T>(mesh, vertex_src_path, fragment_src_path);

        if (logger_.initialized()) {
            auto dummy = T();
            auto sl = dummy.static_log();
            logger_.create_object_group(dummy.type_name(), dummy.log().size(), sl.size());
            logger_.queue_log_object_data(dummy.type_name(), sl, false);
        }
    }

    // add a simobject of a registered type to the simulation
    // the object passed is copied, and management is taken over by the instancer
    // if the simulation was set up to log, also logs object addition
    template<class T>
    void add_object(const T& obj) {
#pragma omp critical
        {
            object_instancer_.add_object(obj);

            if (logger_.initialized()) {
                logger_.queue_new_object(obj.type_name(), obj.get_id());
            }
        }
    }

    // start running the simulation
    void run();

    [[nodiscard]] Vector3 get_world_size() const { return world_size_; }
};
} // swarmulator

#endif //SWARMULATOR_CPP_SIMULATION_H