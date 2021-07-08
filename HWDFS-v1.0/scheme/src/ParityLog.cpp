#include "ParityLog.h"

void ParityLog::writeParity(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    TSCNS ts;
    ts.init();

    // 将更新数据追加到日志中
    uint64_t write_begin = ts.rdns();
    char* temp = new char[request.length];
    memcpy(temp, &(request.data[0]), request.length);
    int64_t curr_offset = 0;
    Manage::writeLog(setting->servers[request.to_id]->path + "/user_data.log",
                     temp, request.length, &curr_offset);
    delete [] temp;
    uint64_t write_end = ts.rdns();

    //临时存放
    uint64_t log_begin = ts.rdns();
    int64_t pn = request.length / setting->part;
    for (int i = request.partId; i < request.partId + pn; ++i) {
        buffer->parity_dr_logs[request.to_id][i].push_back(Log(i, curr_offset, ts.rdns()));
        curr_offset += setting->part;
    }
    uint64_t log_end = ts.rdns();

    _return.code = ResponseCode::SUCCESS;
    cout << "================================" << endl;
    cout << "write time:\t" << write_end - write_begin << endl;
    cout << "record time:\t" << log_end - log_begin << endl;
    cout << "================================\n" << endl;
}

void ParityLog::readParity(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    if(request.type == comm::DataType::P0) {
        char* temp = new char[request.length];
        Manage::readData(setting->servers[request.to_id]->path + "/user_data.bin",
                         temp, request.partId * setting->part, request.length);
        _return.data = CVec(temp, temp + request.length);
        _return.code = ResponseCode::SUCCESS;
        delete [] temp;
    } else if(request.type == comm::DataType::PR) {
        // 读取原始数据P0
        char * data_temp = new char[request.length];
        Manage::readData(setting->servers[request.to_id]->path + "/user_data.bin",
                         data_temp, request.partId * setting->part, request.length);

        // 读取奇偶校验增量
        int fid = ::open((setting->servers[request.to_id]->path + "/user_data.log").c_str(), O_RDONLY, 0777);
        int pn = request.length / setting->part;
        char * temp = new char[setting->part];
        for (int i = request.partId; i < request.partId + pn; ++i) {
            int64_t index = (i - request.partId) * setting->part;
            for (int j = 0; j < buffer->parity_dr_logs[request.to_id][i].size(); ++j) {
                Log log_temp = buffer->parity_dr_logs[request.to_id][i][j];
                ::lseek(fid, log_temp.offset, SEEK_SET);
                ::read(fid, temp, setting->part);

                for (int l = 0; l < setting->part; ++l) {
                    data_temp[index + l] = Galois::add(data_temp[index +l], temp[l]);
                }
            }
        }

        ::close(fid);
        _return.data = CVec(data_temp, data_temp + request.length);
        _return.code = ResponseCode::SUCCESS;
        delete [] temp;
        delete [] data_temp;
    }
}

void ParityLog::degrade_read(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    // 采用传统的纠删码解码的方式恢复数据
    CVec2D shards(setting->total);
    BVec presents(setting->total);

    boost::thread_group group;
    Response responses[setting->total];

    // 获取未丢失的数据
    Request req(request);
    req.from_id = request.to_id;
    responses[request.to_id].code = ResponseCode::ERROR;
    for (int i = 0; i < setting->total; ++i) {
        if(i != request.to_id) {
            //封装Request
            req.to_id = i;
            if(i >= setting->data) {
                req.type = DataType::PR;
            }

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

void ParityLog::mergeParity(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    char* temp = new char[setting->part];
    char* delta = new char[setting->part];

    int fid = ::open((setting->servers[request.to_id]->path + "/user_data.log").c_str(), O_RDONLY, 0777);

    for (int i = 0; i < setting->part_num; i++) {
        for (int j = 0; j < buffer->parity_dr_logs[request.to_id][i].size(); ++j) {
            Log log_temp = buffer->parity_dr_logs[request.to_id][i][j];

            //read p0
            Manage::readData(setting->servers[request.to_id]->path + "/user_data.bin",
                             temp, log_temp.pid * setting->part, setting->part);

            //read delta
            ::lseek(fid, log_temp.offset, SEEK_SET);
            ::read(fid, delta, setting->part);

            // compute pr
            for (int i = 0; i < setting->part; ++i) {
                temp[i] = Galois::add(temp[i], delta[i]);
            }

            //update parity
            Manage::writeChunk(setting->servers[request.to_id]->path + "/user_data.bin",
                               temp, log_temp.pid * setting->part, setting->part);
        }

    }
    ::close(fid);
    delete [] temp;
    delete [] delta;
    buffer->clearParityBuffer();
    _return.code = ResponseCode::SUCCESS;
}
