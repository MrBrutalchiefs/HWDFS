#pragma once

#include "RSConfiguration.h"
#include "Setting.h"
#include "Manage.h"

class MetaManage {
public:
    Manage* manage = NULL;

    MetaManage(const char* config);
    ~MetaManage();

    void create(const char* data, unsigned long length);
    void write(const char* data, unsigned long offset, unsigned long length);
    void read(char* data, unsigned long offset, unsigned long length);
    void repair(char* data, unsigned long offset, unsigned long length, DataType::type type);
    void merge();
    void clear();

private:

};
