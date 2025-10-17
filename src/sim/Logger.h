//
// Created by moltma on 10/17/25.
//

#ifndef SWARMULATOR_CPP_LOGGER_H
#define SWARMULATOR_CPP_LOGGER_H
#include <H5Cpp.h>
#include <iostream>
#include <mutex>
#include <ostream>

namespace swarmulator {
    /*
     * what does the logger have to do?
     * - manage creation and access of the log tables in a threadsafe way
     * - probably just use a mutex instead of omp critical, that's for sure safe everywhere
     * - simobjects have a log method that the simulator calls when that object should dump its state to log
     * - provide methods here which enable saving aribtrary information in arbitrary tables
     * - tables can all contain floats
     * - tables should be built according to the following structure
     * simulation
     * |- objects
     * |    |- <object type name>
     * |        |- state
     * |        |    |- dynamic -> <table with header like: log id, id, pos_x, pos_y, pos_z, etc. all entries collected at a given time must be contiguous>
     * |        |    |- static -> <table with simobject params that were static throughout the simulation>
     * |        |- index
     * |        |    |- time -> <table mapping log ids to their starting positions and segment lengths in the state table>
     * |        |    |- object -> <table mapping object ids to their indexes in the state table, will need to be built offline>
     * |        |- meta
     * |            |- time -> <table mapping log ids to simulation times>
     * |            |- object -> <list of all unique object ids used for this type>
     * |- static -> <table of simulation parameters which did not change over time>
     * |- dynamic -> <table of simulation parameters which changed over time>
     *
     * you'd better read the hdf5 docs!
     */
    class Logger {
    private:
        H5::H5File file_;
        size_t max_entries_;
        const hsize_t chunk_size_ = 1024;
        std::mutex lock_; // use std::lock_guard lock(lock_) to lock the mutex at the beginning of a scope

        H5::Group root_group_;
        H5::Group sim_objects_;
        H5::Group sim_static_;
        H5::Group sim_dynamic_;

    public:
        explicit Logger(size_t max_log_entries = 0, const std::string& path="logfile.h5");
        ~Logger() = default;

        void set_static_str(std::string& str);
        void set_static_int(int i);
        void set_static_float(float f);
    };

} // namespace swarmulator

#endif // SWARMULATOR_CPP_LOGGER_H
