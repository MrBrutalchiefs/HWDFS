#pragma once

#include <cstdint>

class Log {
public:
    int8_t sid;
    int32_t pid;
    int64_t offset;
    int64_t time;
    int8_t type;

    Log() {}

    Log(int32_t pid, int64_t offset, int64_t time) {
        this->pid = pid;
        this->offset = offset;
        this->time = time;
    }

    Log(int8_t sid, int32_t pid, int64_t offset, int64_t time, int8_t type) {
        this->sid = sid;
        this->pid = pid;
        this->offset = offset;
        this->time = time;
        this->type = type;
    }
};

