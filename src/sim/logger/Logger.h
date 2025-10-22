//
// Created by moltma on 10/17/25.
//

#ifndef SWARMULATOR_CPP_LOGGER_H
#define SWARMULATOR_CPP_LOGGER_H
#include <H5Cpp.h>
#include <atomic>
#include <iostream>
#include <map>
#include <vector>

#include "../SimObject.h"
#include "LogTask.h"
#include "ThreadsafeQueue.h"

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
     * |        |    |- dynamic -> <table mapping log id to object id, pos_x, pos_y, pos_z, etc. all entries collected at a given time must be contiguous>
     * |        |    |- static -> <table with simobject params that were static throughout the simulation>
     * |        |- index -> <table mapping log ids to their starting positions and segment lengths in the state table>
     * |        |- meta
     * |            |- object -> <list of all unique object ids used for this type>
     * |- static -> <table of simulation parameters which did not change over time>
     * |- dynamic -> <table of simulation parameters which changed over time>
     * |- time -> <table mapping log ids to simulation times>
     *
     * you'd better read the hdf5 docs!
     *
     * also!
     * the logger is based on a worker thread model. writes have to be serialized anyways, so the simulation enqueues logging tasks and the logger, running in its own thread, has plenty of time to get them done serially
     * because we enqueue, order of log entries is preserved
     */
    class Logger {
    private:
        struct object_group {
            H5::DataSet state_dynamic;
            H5::DataSet state_static;
            H5::DataSet index;
            H5::DataSet meta_object;
        };

        H5::H5File file_; // logfile to write to

        // map object type names to their datasets
        std::map<std::string, object_group> object_groups_; // keeping dataset handles in memory is much faster than querying/opening every time
        H5::Group sim_objects_;
        // simulation properties static table
        H5::DataSet sim_static_;
        // simulation properties dynamic table
        H5::DataSet sim_dynamic_;
        // time map table info
        H5::DataSet sim_time_;

        size_t frame_id_ = 0; // current log entry id

        // stuff for the worker thread model
        ThreadsafeQueue<LogTask*> task_queue_;
        std::thread worker_thread_;
        std::atomic<bool> want_exit_;

        // state info
        bool initialized_ = false;
        // parameters
        size_t max_entries_; // knowable since simulation lengths are bounded
        static constexpr hsize_t chunk_size_ = 1024;
        size_t compression_level_ = 0; // 0 to 9

        // write a row of floats to a known-length dataset
        static void write_frow(const size_t idx, const std::vector<float>& values, const H5::DataSet& dataset) {
            const auto filespace = dataset.getSpace();
            const hsize_t offset[2] = {idx, 0};
            const hsize_t count[2] = {1, values.size()};
            filespace.selectHyperslab(H5S_SELECT_SET, count, offset);
            const auto memspace = H5::DataSpace(2, count);
            dataset.write(values.data(), H5::PredType::NATIVE_FLOAT, memspace, filespace);
        }

        // append a row of floats to an unlimited-length dataset
        static void app_frow(const std::vector<float>& values, const H5::DataSet& dataset) {
            hsize_t dims_current[2];
            dataset.getSpace().getSimpleExtentDims(dims_current);
            hsize_t rows = dims_current[0];
            hsize_t new_rows = rows + 1;

            hsize_t dims_new[2] = { new_rows, dims_current[1] };
            dataset.extend(dims_new);

            const auto filespace = dataset.getSpace();
            hsize_t offset[2] = { rows, 0, };
            hsize_t count[2] = { 1, values.size() };
            filespace.selectHyperslab(H5S_SELECT_SET, count, offset);

            const auto memspace = H5::DataSpace(2, count);
            dataset.write(values.data(), H5::PredType::NATIVE_FLOAT, memspace, filespace);
        }

        // write a row of integers to a known-length dataset
        static void write_irow(const size_t idx, const std::vector<int>& values, const H5::DataSet& dataset) {
            const auto filespace = dataset.getSpace();
            const hsize_t offset[2] = {idx, 0};
            const hsize_t count[2] = {1, values.size()};
            filespace.selectHyperslab(H5S_SELECT_SET, count, offset);
            const auto memspace = H5::DataSpace(2, count);
            dataset.write(values.data(), H5::PredType::NATIVE_INT, memspace, filespace);
        }

        // append a row of integers to an unlimited-length dataset
        static void app_irow(const std::vector<int>& values, const H5::DataSet& dataset) {
            hsize_t dims_current[2];
            dataset.getSpace().getSimpleExtentDims(dims_current);
            hsize_t rows = dims_current[0];
            hsize_t new_rows = rows + 1;

            hsize_t dims_new[2] = { new_rows, dims_current[1] };
            dataset.extend(dims_new);

            const auto filespace = dataset.getSpace();
            hsize_t offset[2] = { rows, 0, };
            hsize_t count[2] = { 1, values.size() };
            filespace.selectHyperslab(H5S_SELECT_SET, count, offset);

            const auto memspace = H5::DataSpace(2, count);
            dataset.write(values.data(), H5::PredType::NATIVE_INT, memspace, filespace);
        }

        // read a row of integers from a dataset
        static std::vector<int> read_irow(const size_t idx, const H5::DataSet& dataset) {
            const auto filespace = dataset.getSpace();
            hsize_t dims[2];
            filespace.getSimpleExtentDims(dims);
            const hsize_t offset[2] = {idx, 0};
            const hsize_t count[2] = {1, dims[1]};
            filespace.selectHyperslab(H5S_SELECT_SET, count, offset);
            const auto memspace = H5::DataSpace(2, count);
            int out[dims[1]];
            dataset.read(out, H5::PredType::NATIVE_INT, memspace, filespace);
            return std::vector<int>(out, out+dims[1]);
        }

        void worker_loop();

        void init_guard() const {
            if (!initialized_) {
                throw std::runtime_error("Logger was not initialized.");
            }
        }

    public:
        Logger() = default;
        ~Logger();

        void initialize(const std::string& path, size_t deflate_level, size_t max_entries, size_t static_sim_entry_width, size_t dynamic_sim_entry_width);

        // create the h5 group for an object type (state, index, meta subgroups)
        // does nothing if the group exists
        // also initializes index/time, index/object, meta/object since we already know the shapes of those
        void create_object_group(const std::string &name, size_t object_dynamic_log_width, size_t object_static_log_width);

        // because the logger runs in its own thread, all you can do is en/dequeue logging tasks
        // task order is preserved
        // workflow:
        // begin frame (once per update)
        // log dynamic data (many times per update)
        // advance frame (once per update)
        void queue_begin_frame(float real_time);
        void queue_advance_frame();
        void queue_log_object_data(const std::string &object_type_name, const std::vector<float> &vals, bool dynamic);
        void queue_new_object(const std::string &object_type_name, size_t id);
        void queue_log_sim_data(std::vector<float> vals, bool dynamic);

        [[nodiscard]] std::size_t tasks_queued() { return task_queue_.size(); }
        [[nodiscard]] bool initialized() const { return initialized_; }
    };

} // namespace swarmulator

#endif // SWARMULATOR_CPP_LOGGER_H
