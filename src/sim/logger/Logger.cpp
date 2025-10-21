//
// Created by moltma on 10/17/25.
//

#include "Logger.h"

namespace swarmulator {
    void Logger::worker_loop() {
        LogTask* task;
        while (task_queue_.pop(task)) {
            if (const auto s = task_queue_.size(); want_exit_ && s % 1024 * 1024 == 0) {
                std::cout << "\r" << s << " logging tasks left." << std::flush;
            }

            // check if our task is to begin a new frame
            if (const auto begin_frame = dynamic_cast<BeginFrame*>(task); begin_frame != nullptr) {
                write_frow(frame_id_, {begin_frame->real_time}, sim_time_);

                for (const auto& [gname, group] : object_groups_) {
                    if (frame_id_ == 0) {
                        write_irow(0, {0, 0}, group.index);
                    }
                    else {
                        auto last = read_irow(frame_id_ - 1, group.index);
                        write_irow(frame_id_, {last[0] + last[1]}, group.index);
                    }
                }
            }
            // check if our task is to advance to a new frame
            else if (const auto advance_frame = dynamic_cast<AdvanceFrame*>(task); advance_frame != nullptr) {
                frame_id_++;
                if (frame_id_ >= max_entries_) {
                    throw std::runtime_error("Maximum number of log entries exceeded.");
                }
            }
            // check if our task is to log some object data
            else if (const auto log_obj = dynamic_cast<LogObjectData*>(task); log_obj != nullptr) {
                // if we're logging object dynamic data, do that
                if (log_obj->dynamic) {
                    const auto& group = object_groups_[log_obj->object_type_name];
                    // add the new object log to the dynamic log table
                    app_frow(log_obj->values, group.state_dynamic);
                    // update time segment info
                    auto segment_info = read_irow(frame_id_, group.index); // 0 is segment start, 1 is segment length
                    segment_info[1]++; // increment segment length
                    write_irow(frame_id_, segment_info, group.index); // and write back
                }
                // otherwise log static data
                else {
                    auto& grp_info = object_groups_[log_obj->object_type_name];

                    hsize_t dims[2];
                    const auto space = grp_info.state_static.getSpace();
                    space.getSimpleExtentDims(dims);
                    if (dims[1] != log_obj->values.size()) {
                        throw std::runtime_error("Invalid static data width.");
                    }

                    float data_raw[dims[0]][dims[1]];
                    for (int i = 0; i < log_obj->values.size(); i++) {
                        data_raw[0][i] = log_obj->values[i];
                    }

                    grp_info.state_static.write(data_raw, H5::PredType::NATIVE_FLOAT);
                }
            }
            // check if our task is to log the creation of a new object
            else if (const auto new_obj = dynamic_cast<NewObject*>(task); new_obj != nullptr) {
                auto& grp_info = object_groups_[new_obj->object_type_name];
                app_irow({new_obj->id}, grp_info.meta_object);
            }
            // check if our task is to log some sim data
            else if (const auto log_sim = dynamic_cast<LogSimData*>(task); log_sim != nullptr) {
                // if logging dynamic data, just append to dynamic table
                if (log_sim->dynamic) {
                    write_frow(frame_id_, log_sim->values, sim_dynamic_);
                }
                else {
                    hsize_t dims[2];
                    const auto space = sim_static_.getSpace();
                    space.getSimpleExtentDims(dims);
                    if (dims[1] != log_sim->values.size()) {
                        throw std::runtime_error("Invalid static data width.");
                    }

                    float data_raw[dims[0]][dims[1]];
                    for (int i = 0; i < log_sim->values.size(); i++) {
                        data_raw[0][i] = log_sim->values[i];
                    }

                    sim_static_.write(data_raw, H5::PredType::NATIVE_FLOAT);
                }
            }
            else {
                throw std::runtime_error("Unknown logging task.");
            }

            // once we're done we can delete the task
            delete task;
        }
    }

    Logger::Logger(const size_t max_log_entries, const std::string &path, const size_t static_width, const size_t dynamic_width) {
        max_entries_ = max_log_entries;
        // first thing we can do is set up the basic table structure and create the file
        file_ = H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
        root_group_ = file_.createGroup("simulation");
        sim_objects_ = root_group_.createGroup("objects");

        // set up time index table
        hsize_t dims[2];
        dims[0] = max_entries_; // index is log entry id
        dims[1] = 1; // value is real sim time
        auto space = H5::DataSpace(2, dims);
        sim_time_ = root_group_.createDataSet("time", H5::PredType::NATIVE_FLOAT, space);

        // static property table
        dims[0] = 1;
        dims[1] = static_width;
        space = H5::DataSpace(2, dims);
        sim_static_ = root_group_.createDataSet("static", H5::PredType::NATIVE_FLOAT, space);

        // dynamic property table
        dims[0] = max_entries_;
        dims[1] = dynamic_width;
        space = H5::DataSpace(2, dims);
        sim_dynamic_ = root_group_.createDataSet("dynamic", H5::PredType::NATIVE_FLOAT, space);

        // start up the worker loop
        worker_thread_ = std::thread(&Logger::worker_loop, this);
        want_exit_ = false;
    }

    Logger::~Logger() {
        // destructor just waits for the worker to finish logging
        std::cout << "Finishing logs..." << std::endl;
        want_exit_ = true; // tell the worker we want to be done
        task_queue_.stop(); // tell the queue there won't be any more things coming in
        worker_thread_.join(); // wait for the worker to finish processing all the accumulated log entries
    }

    void Logger::create_object_group(const std::string &name, const size_t object_dynamic_log_width, size_t object_static_log_width) {
        if (sim_objects_.exists(name)) {
            return;
        }

        const auto object_group = sim_objects_.createGroup(name);
        struct object_group mem_group;
        // create state group
        const auto state_group = object_group.createGroup("state");
        // dynamic state table
        hsize_t dims[2] = {0, object_dynamic_log_width}; // dims[0] is rows, dims[1] is cols
        hsize_t maxdims[2] = {H5S_UNLIMITED, object_dynamic_log_width};
        // because the dynamic table is unlimited, we have to create a proplist and set the chunking
        auto plist = H5::DSetCreatPropList();
        hsize_t chunk_dims[2] = {chunk_size_, object_dynamic_log_width}; // chunk into however many rows per read
        plist.setChunk(2, chunk_dims);
        auto space = H5::DataSpace(2, dims, maxdims);
        const auto dynamic_state = state_group.createDataSet("dynamic", H5::PredType::NATIVE_FLOAT, space, plist);

        // static state table
        dims[0] = 1; // these are parameters that do not change for this object group, so we only need one row
        dims[1] = {object_static_log_width};
        space = H5::DataSpace(2, dims);
        const auto static_state = state_group.createDataSet("static", H5::PredType::NATIVE_FLOAT, space);

        // index/time segment table
        dims[0] = max_entries_; // index is log entry id
        dims[1] = 2; // value[0] is start of time segment in state table, value[1] is length of time segment in state table
        space = H5::DataSpace(2, dims);
        const auto time_idx = object_group.createDataSet("index", H5::PredType::NATIVE_INT, space);

        // set up meta group
        const auto meta_group = object_group.createGroup("meta");
        // all object ids table
        dims[0] = 0;
        dims[1] = 1;
        maxdims[0] = H5S_UNLIMITED;
        maxdims[1] = 1;
        space = H5::DataSpace(2, dims, maxdims);
        plist = H5::DSetCreatPropList();
        chunk_dims[0] = chunk_size_;
        chunk_dims[1] = 1;
        plist.setChunk(2, chunk_dims);
        const auto object_ids = meta_group.createDataSet("object", H5::PredType::NATIVE_INT, space, plist);

        mem_group.state_dynamic = dynamic_state;
        mem_group.state_static = static_state;
        mem_group.index = time_idx;
        mem_group.meta_object = object_ids;
        object_groups_.insert(std::make_pair(name, mem_group));
    }

    void Logger::queue_begin_frame(const float real_time) {
        const auto task = new BeginFrame();
        task->real_time = real_time;
        task_queue_.push(task);
    }

    void Logger::queue_advance_frame() {
        task_queue_.push(new AdvanceFrame());
    }

    void Logger::queue_log_object_data(const std::string &object_type_name, const std::vector<float> &vals, const bool dynamic) {
        const auto task = new LogObjectData();
        task->object_type_name = object_type_name;
        task->values = vals;
        task->dynamic = dynamic;
        task_queue_.push(task);
    }

    void Logger::queue_new_object(const std::string &object_type_name, const size_t id) {
        const auto task = new NewObject();
        task->object_type_name = object_type_name;
        task->id = static_cast<int>(id);
        task_queue_.push(task);
    }

    void Logger::queue_log_sim_data(std::vector<float> vals, bool dynamic) {
        const auto task = new LogSimData();
        task->values = vals;
        task->dynamic = dynamic;
        task_queue_.push(task);
    }
} // namespace swarmulator
