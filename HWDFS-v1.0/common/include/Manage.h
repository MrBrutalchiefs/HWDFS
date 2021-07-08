#pragma once
#include <fcntl.h>
#include <boost/thread/thread.hpp>
#include <boost/thread/detail/thread_group.hpp>

#include "Setting.h"
#include "RSConfiguration.h"
#include "ReedSolomon.h"
#include "TSCNS.h"
#include "Log.h"

using namespace comm;

class Manage {
public:
    Setting* setting = NULL;

    Manage(const char* config);
    ~Manage();

    void create(const char* data, unsigned long length);
    void write(const char* data, unsigned long offset, unsigned long length);
    void read(char* data, unsigned long offset, unsigned long length);
    void repair(char* data, unsigned long offset, unsigned long length, DataType::type type);
    void merge();
    void clear();

    static void sendDataToServer(Response* response, Request request, StoreServiceClient* client, string type);
    static void writeLog(string path, char * data, int64_t length, int64_t * curr_offset);
    static void writeChunk(string path, char * data, int64_t offset, int64_t length);
    static void readData(string path, char * data, int64_t offset, int64_t length);
};

