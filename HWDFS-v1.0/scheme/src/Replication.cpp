#include "Replication.h"

void Replication::writeData(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    TSCNS ts;
    ts.init();

    // 并行更新数据
    uint64_t write_begin = ts.rdns();
    Response responses[setting->parity];
    Request req(request);
    req.from_id = request.to_id;
    char* temp = new char[request.length];
    memcpy(temp, &(request.data[0]), request.length);
    boost::thread_group group;
    group.add_thread(new boost::thread(Manage::writeChunk,
                                       setting->servers[request.to_id]->path + "/user_data.bin",
                                       temp, request.partId * setting->part, request.length));
    for (int i = 0; i < setting->parity; ++i) {
        req.to_id = i + setting->data;
        group.add_thread(new boost::thread(Manage::sendDataToServer, &responses[i],
                                           req, setting->clients[req.to_id], "WRITE"));
    }
    group.join_all();
    delete [] temp;
    uint64_t write_end = ts.rdns();

    _return.code = ResponseCode::SUCCESS;
    cout << "================================" << endl;
    cout << "write dr time:\t" << write_end - write_begin << endl;
    cout << "================================\n" << endl;
}

void Replication::writeParity(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    char* temp = new char[request.length];
    memcpy(temp, &(request.data[0]), request.length);
    Manage::writeChunk(setting->servers[request.to_id]->path + "/user_data.bin",
                       temp, request.partId * setting->part, request.length);
    delete [] temp;
    _return.code = ResponseCode::SUCCESS;
}

void Replication::degrade_read(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    Request req(request);
    req.from_id = request.to_id;
    for (int i = setting->data; i < setting->total; ++i) {
        req.to_id = i;
        Manage::sendDataToServer(&_return, req, setting->clients[i], "READ");
        if(_return.code == ResponseCode::SUCCESS) {
            return;
        }
    }
    _return.code = ResponseCode::ERROR;
    return;
}