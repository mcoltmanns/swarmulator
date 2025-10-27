//
// Created by moltma on 10/21/25.
//

#ifndef SWARMULATOR_CPP_LOGTASK_H
#define SWARMULATOR_CPP_LOGTASK_H
#include <string>
#include <vector>

namespace swarmulator {
    class LogTask {
    public:
        virtual ~LogTask() = default;
    };

    class BeginFrame final : public LogTask {
    public:
        float real_time;

        ~BeginFrame() override = default;
    };

    class AdvanceFrame final : public LogTask {
    public:
        ~AdvanceFrame() override = default;
    };

    class LogObjectData final : public LogTask {
    public:
        bool dynamic;
        std::string object_type_name;
        std::vector<float> values;

        ~LogObjectData() override = default;
    };

    class NewObject final : public LogTask {
    public:
        std::string object_type_name;
        size_t id;
    };

    class LogSimData final : public LogTask {
    public:
        bool dynamic;
        std::vector<float> values;

        ~LogSimData() override = default;
    };
}

#endif // SWARMULATOR_CPP_LOGTASK_H
