#pragma once

#include "Scheme.h"

class Replication: public Scheme {
public:

    void writeData(Response &_return, const Request &request, Buffer* buffer, Setting* setting);
    void writeParity(Response &_return, const Request &request, Buffer* buffer, Setting* setting);
    void degrade_read(Response &_return, const Request &request, Buffer* buffer, Setting* setting);
};