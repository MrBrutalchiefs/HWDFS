#pragma once

#include "Scheme.h"

class PWDR : public Scheme {
public:

    void writeData(Response& _return, const Request& request, Buffer* buffer, Setting* setting);
    void writeParity(Response& _return, const Request& request, Buffer* buffer, Setting* setting);
    void readData(Response& _return, const Request& request, Buffer* buffer, Setting* setting);
    void readParity(Response& _return, const Request& request, Buffer* buffer, Setting* setting);
    void degrade_read(Response& _return, const Request& request, Buffer* buffer, Setting* setting);
    void mergeData(Response& _return, const Request& request, Buffer* buffer, Setting* setting);
    void mergeParity(Response& _return, const Request& request, Buffer* buffer, Setting* setting);
};