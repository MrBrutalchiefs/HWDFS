#pragma once

#include "Scheme.h"

class ParityLog : public Scheme {
public:

    void writeParity(Response& _return, const Request& request, Buffer* buffer, Setting* setting);
    void readParity(Response &_return, const Request &request, Buffer* buffer, Setting* setting);
    void degrade_read(Response& _return, const Request& request, Buffer* buffer, Setting* setting);
    void mergeParity(Response& _return, const Request& request, Buffer* buffer, Setting* setting);
};