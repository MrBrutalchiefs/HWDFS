#include "Scheme.h"

void Scheme::create(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    char * temp = new char[request.length];
    memcpy(temp, &(request.data[0]), request.length);
    Manage::writeChunk(setting->servers[request.to_id]->path + "/user_data.bin",
                       temp, request.partId * setting->part, request.length);
    delete [] temp;
    _return.code = ResponseCode::SUCCESS;
}

void Scheme::writeData(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    TSCNS ts;
    ts.init();

    //读取d(r-1)
    uint64_t read_begin = ts.rdns();
    char* data = new char[request.length];
    Manage::readData(setting->servers[request.to_id]->path + "/user_data.bin",
                     data, request.partId * setting->part, request.length);
    uint64_t read_end = ts.rdns();

    //计算增量(d(r)-d(r-1)) * gamma
    uint64_t compute_begin = ts.rdns();
    Request reqs[setting->parity];
    for (int i = 0; i < setting->parity; ++i) {
        // 封装Request
        reqs[i] = Request();
        reqs[i].from_id = request.to_id;
        reqs[i].to_id = setting->data + i;
        reqs[i].scheme = request.scheme;
        reqs[i].partId = request.partId;
        reqs[i].offset = request.offset;
        reqs[i].length = request.length;
        reqs[i].type = comm::DataType::PR;

        int8_t gamma = setting->rs->parityRows[i][request.to_id];
        reqs[i].data = CVec(request.length);
        for (int j = 0; j < request.length; ++j) {
            reqs[i].data[j] = Galois::multiply(Galois::subtract(request.data[j], data[j]), gamma);
        }
    }
    uint64_t compute_end = ts.rdns();

    // 更新数据
    uint64_t write_begin = ts.rdns();
    Response responses[setting->parity];
    boost::thread_group group;
    memcpy(data, &(request.data[0]), request.length);
    group.add_thread(new boost::thread(Manage::writeChunk,
                                       setting->servers[request.to_id]->path + "/user_data.bin",
                                       data, request.partId * setting->part, request.length));
    for (int i = 0; i < setting->parity; ++i) {
        group.add_thread(new boost::thread(Manage::sendDataToServer, &responses[i], reqs[i],
                                           setting->clients[reqs[i].to_id], "WRITE"));
    }
    group.join_all();
    delete [] data;
    uint64_t write_end = ts.rdns();

    _return.code = ResponseCode::SUCCESS;
    cout << "================================" << endl;
    cout << "read time:\t" << (read_end - read_begin)<< endl;
    cout << "compute time:\t" << compute_end - compute_begin << endl;
    cout << "write time:\t" << write_end - write_begin << endl;
    cout << "================================\n" << endl;
}

void Scheme::writeParity(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    //读取p(r-1)
    char* parity = new char[request.length];
    Manage::readData(setting->servers[request.to_id]->path + "/user_data.bin",
                     parity, request.partId * setting->part, request.length);

    //计算pr
    for (int i = 0; i < request.length; ++i) {
        parity[i] = Galois::add(parity[i], request.data[i]);
    }

    //更新奇偶校验块
    Manage::writeChunk(setting->servers[request.to_id]->path + "/user_data.bin",
                       parity, request.partId * setting->part, request.length);

    _return.code = ResponseCode::SUCCESS;
    delete [] parity;
}

void Scheme::readData(Response& _return, const Request& request, Buffer* buffer, Setting* setting) {
    //读取dr
    char* data = new char[request.length];
    Manage::readData(setting->servers[request.to_id]->path + "/user_data.bin",
                     data, request.partId * setting->part, request.length);
    _return.data = CVec(data, data + request.length);
    _return.code = ResponseCode::SUCCESS;
    delete [] data;
}

void Scheme::readParity(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    readData(_return, request, buffer, setting);
}

void Scheme::degrade_read(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    // 采用传统的纠删码解码的方式恢复数据
    CVec2D shards(setting->total);
    BVec presents(setting->total);

    boost::thread_group group;
    Response responses[setting->total];

    // 获取未丢失的数据
    //封装Request
    Request req(request);
    req.from_id = request.to_id;
    responses[request.to_id].code = ResponseCode::ERROR;
    for (int i = 0; i < setting->total; ++i) {
        if(i != request.to_id) {
            req.to_id = i;
            group.add_thread(new boost::thread(Manage::sendDataToServer, &responses[i], req,
                                               setting->clients[i], "READ"));
        }
    }
    group.join_all();

    int64_t index = request.partId * setting->part;
    for (int i = 0; i < setting->total; ++i) {
        shards[i] = CVec(setting->size);
        if(responses[i].code == ResponseCode::SUCCESS) {
            for (int j = 0; j < request.length; ++j) {
                shards[i][j + index] = responses[i].data[j];
            }
            presents[i] = true;
        } else {
            presents[i] = false;
        }
    }

    // 解码
    setting->rs->decodeMissing(shards, presents, index, request.length);

    _return.data = CVec(shards[request.to_id].begin() + index,
                        shards[request.to_id].begin() + index + request.length);
    _return.code = ResponseCode::SUCCESS;
}

void Scheme::clear(Response &_return, const Request &request, Buffer *buffer, Setting *setting) {
    ::remove((setting->servers[request.to_id]->path + "/user_data.bin").c_str());
    ::remove((setting->servers[request.to_id]->path + "/user_data.log").c_str());
    buffer->clearDataBuffer();
    buffer->clearParityBuffer();
    _return.code = ResponseCode::SUCCESS;
}
