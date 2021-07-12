#pragma once

#include <string>
#include <pthread.h>
#include "Manage.h"
#include "Buffer.h"

/**
 * Default, SuperClass
 * Full-Overwrite Scheme
 */
class Scheme {
public:
    /**
     * Create Chunks
     * @param _return
     * @param request
     */
    virtual void create(Response& _return, const Request& request, Buffer* buffer, Setting* setting);

    /**
     * Update Data Chunk
     * @param _return
     * @param request
     */
    virtual void writeData(Response& _return, const Request& request, Buffer* buffer, Setting* setting);

    /**
     * Update Parity Chunk
     * @param _return
     * @param request
     */
    virtual void writeParity(Response& _return, const Request& request, Buffer* buffer, Setting* setting);

    /**
     * Read Data Chunk
     * @param _return
     * @param request
     */
    virtual void readData(Response& _return, const Request& request, Buffer* buffer, Setting* setting);

    /**
     * Read Parity Chunk
     * @param _return
     * @param request
     */
    virtual void readParity(Response& _return, const Request& request, Buffer* buffer, Setting* setting);

    /**
     * Repair Data Chunk
     * @param _return
     * @param request
     */
    virtual void degrade_read(Response& _return, const Request& request, Buffer* buffer, Setting* setting);

    /**
     * Replay Data Journal
     * @param _return
     */
    virtual void mergeData(Response& _return, const Request& request, Buffer* buffer, Setting* setting) {}

    /**
     * Replay Parity Journal
     * @param _return
     */
    virtual void mergeParity(Response& _return, const Request& request, Buffer* buffer, Setting* setting) {}

    /**
     * Delete User Data And Log
     * @param _return
     */
    virtual void clear(Response& _return, const Request& request, Buffer* buffer, Setting* setting);
};
