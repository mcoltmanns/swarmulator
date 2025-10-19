//
// Created by moltma on 10/17/25.
//

#include "Logger.h"

namespace swarmulator {
    Logger::Logger(const size_t max_log_entries, const std::string &path) {
        std::unique_lock lock(lock_);
        max_entries_ = max_log_entries;
        // first thing we can do is set up the basic table structure and create the file
        file_ = H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
        root_group_ = file_.createGroup("simulation");
        sim_objects_ = root_group_.createGroup("objects");
        sim_static_ = root_group_.createGroup("static");
        sim_dynamic_ = root_group_.createGroup("dynamic");
        sim_time_ = root_group_.createGroup("time");
    }
} // namespace swarmulator
