#include "DDBHW.h"

void DDBHW::writeData(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    TSCNS ts;
    ts.init();

    // 判断数据是否为第一次更新
    uint64_t judge_begin = ts.rdns();
    int pn = request.length / setting->part;
    std::string key = to_string(request.partId) + "-" + to_string(pn);
    bool is_first = !buffer->data_is_update[key];
    buffer->data_is_update[key] = true;
    uint64_t judge_end = ts.rdns();

    // 进行更新
    if(is_first) {
        uint64_t dr_begin = ts.rdns();
        Response responses[setting->parity];
        char* temp = new char[request.length];
        char* d0 = new char [request.length];
        memcpy(temp, &(request.data[0]), request.length);
        Request req(request);
        req.from_id = request.to_id;
        // 发送更新数据，并读出原始数据
        boost::thread_group group1;
        group1.add_thread(new boost::thread(Manage::readData,
                                           setting->servers[request.to_id]->path + "/user_data.bin",
                                           d0, request.partId * setting->part, request.length));
        for (int i = 0; i < setting->parity; ++i) {
            req.to_id = i + setting->data;
            group1.add_thread(new boost::thread(Manage::sendDataToServer, &responses[i],
                                               req, setting->clients[req.to_id], "WRITE"));
        }
        group1.join_all();
        uint64_t dr_end = ts.rdns();

        // 发送原始数据，并将更新数据写入日志
        uint64_t d0_begin = ts.rdns();
        int64_t current_offset = 0;
        req.type = comm::DataType::D0;
        req.data = CVec(d0, d0 + request.length);
        boost::thread_group group2;
        group2.add_thread(new boost::thread(Manage::writeLog,
                                           setting->servers[request.to_id]->path + "/user_data.log",
                                           temp, request.length, &current_offset));
        for (int i = 0; i < setting->parity; ++i) {
            req.to_id = i + setting->data;
            group2.add_thread(new boost::thread(Manage::sendDataToServer, &responses[i],
                                               req, setting->clients[req.to_id], "WRITE"));
        }
        group2.join_all();
        delete [] temp;
        delete [] d0;
        uint64_t d0_end = ts.rdns();

        // 日志记录
        uint64_t record_begin = ts.rdns();
        int pn = request.length / setting->part;
        for (int i = request.partId; i < request.partId + pn; ++i) {
            buffer->data_dr_logs[i].push_back(Log(i, current_offset, ts.rdns()));
            current_offset += setting->part;
            buffer->data_site[i] = true;       // 标记最新数据在日志中
        }
        uint64_t record_end = ts.rdns();
        _return.code = ResponseCode::SUCCESS;

        cout << "================================" << endl;
        cout << "judge time:\t" << judge_end - judge_begin << endl;
        cout << "write dr time:\t" << dr_end - dr_begin << endl;
        cout << "write d0 time:\t" << d0_end - d0_begin << endl;
        cout << "record time:\t" << record_end - record_begin << endl;
        cout << "================================\n" << endl;
    } else {
        uint64_t write_begin = ts.rdns();
        // 标记最新数据位于块中
        int pn = request.length / setting->part;
        for (int i = request.partId; i < request.partId + pn; ++i) {
            buffer->data_site[i] = false;
        }

        // 并行更新数据
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
        cout << "judge time:\t" << judge_end - judge_begin << endl;
        cout << "write dr time:\t" << write_end - write_begin << endl;
        cout << "================================\n" << endl;
    }
}

void DDBHW::writeParity(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    if(request.type == comm::DataType::D0) {
        TSCNS ts;
        ts.init();
        int64_t curr_offset = 0;

        uint64_t write_begin = ts.rdns();
        char * temp = new char[request.length];
        memcpy(temp, &(request.data[0]), request.length);
        Manage::writeLog(setting->servers[request.to_id]->path + "/user_data.log",
                         temp, request.length, &curr_offset);
        delete [] temp;
        uint64_t write_end = ts.rdns();

        uint64_t record_begin = ts.rdns();
        int64_t pn = request.length / setting->part;
        for (int i = request.partId; i < request.partId + pn; ++i) {
            buffer->parity_d0_logs[request.from_id][i].push_back(Log(i, curr_offset, ts.rdns()));
            curr_offset += setting->part;
        }
        uint64_t record_end = ts.rdns();

        _return.code = ResponseCode::SUCCESS;
        cout << "================================" << endl;
        cout << "write time:\t" << write_end - write_begin << endl;
        cout << "record time:\t" << record_end - record_begin << endl;
        cout << "================================\n" << endl;
    } else if(request.type == comm::DataType::DR) {
        TSCNS ts;
        ts.init();
        int64_t curr_offset = 0;

        uint64_t write_begin = ts.rdns();
        char* temp = new char[request.length];
        memcpy(temp, &(request.data[0]), request.length);
        Manage::writeLog(setting->servers[request.to_id]->path + "/user_data.log",
                         temp, request.length, &curr_offset);
        delete [] temp;
        uint64_t write_end = ts.rdns();

        uint64_t record_begin = ts.rdns();
        int64_t pn = request.length / setting->part;
        for (int i = request.partId; i < request.partId + pn; ++i) {
            buffer->parity_dr_logs[request.from_id][i].push_back(Log(i, curr_offset, ts.rdns()));
            curr_offset += setting->part;
        }
        uint64_t record_end = ts.rdns();

        // 查找d0的日志数据是否存在
        uint64_t search_begin = ts.rdns();
        bool flag = false;
        for (int i = request.partId; i < request.partId + pn; ++i) {
            if(buffer->parity_d0_logs[request.from_id][i].empty()) {
                flag = true;
                break;
            }
        }
        if(flag) {
            _return.code = ResponseCode::NEED_D0;
        } else {
            _return.code = ResponseCode::SUCCESS;
        }
        uint64_t search_end = ts.rdns();

        cout << "================================" << endl;
        cout << "write time:\t" << write_end - write_begin << endl;
        cout << "record time:\t" << record_end - record_begin << endl;
        cout << "search time:\t" << search_end - search_begin << endl;
        cout << "================================\n" << endl;
    }
}

void DDBHW::readData(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    char* temp = new char[setting->part];
    int pn = request.length / setting->part;
    int fid_dr = ::open((setting->servers[request.to_id]->path + "/user_data.log").c_str(), O_RDONLY, 0777);
    int fid_d0 = ::open((setting->servers[request.to_id]->path + "/user_data.bin").c_str(), O_RDONLY, 0777);

    for (int i = request.partId; i < request.partId + pn; ++i) {
        if(!buffer->data_site[i]) {
            ::lseek(fid_d0, i * setting->part, SEEK_SET);
            ::read(fid_d0, temp, setting->part);
        } else {
            ::lseek(fid_dr, buffer->data_dr_logs[i].back().offset, SEEK_SET);
            ::read(fid_dr, temp, setting->part);
        }
        _return.data.insert(_return.data.end(), temp, temp + setting->part);
    }
    ::close(fid_d0);
    ::close(fid_dr);
    delete [] temp;
    _return.code = ResponseCode::SUCCESS;
}

void DDBHW::readParity(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    if(request.type == comm::DataType::DR) {
        int pn = request.length / setting->part;
        char * temp = new char[setting->part];

        int fid = ::open((setting->servers[request.to_id]->path + "/user_data.log").c_str(), O_RDONLY, 0777);
        for (int i = request.partId; i < request.partId + pn; ++i) {
            if(!buffer->parity_dr_logs[request.from_id][i].empty()) {
                ::lseek(fid, buffer->parity_dr_logs[request.from_id][i].back().offset, SEEK_SET);
                ::read(fid, temp, setting->part);
                _return.data.insert(_return.data.end(), temp, temp + setting->part);
            } else {
                _return.code = ResponseCode::ERROR;
                ::close(fid);
                delete [] temp;
                return;
            }
        }
        ::close(fid);
        delete [] temp;
        _return.code = ResponseCode::SUCCESS;
    } else if(request.type == comm::DataType::P0) {
        char* temp = new char[request.length];
        Manage::readData(setting->servers[request.to_id]->path + "/user_data.bin",
                         temp, request.partId * setting->part, request.length);
        _return.data = CVec(temp, temp + request.length);
        _return.code = ResponseCode::SUCCESS;
        delete [] temp;
    } else if(request.type == comm::DataType::PR) {
        // 先读取P0
        char* temp = new char[request.length];
        Manage::readData(setting->servers[request.to_id]->path + "/user_data.bin",
                         temp, request.partId * setting->part, request.length);
        _return.data = CVec(temp, temp + request.length);

        // 计算delta_p
        int pn = request.length / setting->part;
        _return.data = CVec(request.length);
        char* d0 = new char[setting->part];
        char* dr = new char[setting->part];
        int fid = ::open((setting->servers[request.to_id]->path + "/user_data.log").c_str(), O_RDONLY, 0777);

        for (int i = 0; i < setting->data; i++) {
            int64_t index = 0;
            int8_t gamma = setting->rs->parityRows[request.to_id - setting->data][i];
            for (int j = request.partId; j < request.partId + pn; ++j) {
                if(!buffer->parity_dr_logs[i][j].empty()) {
                    // 获取dr
                    Log log_dr = buffer->parity_dr_logs[i][j].back();
                    ::lseek(fid, log_dr.offset, SEEK_SET);
                    ::read(fid, dr, setting->part);

                    // 获取d0
                    Log log_d0 = buffer->parity_d0_logs[i][j].back();
                    ::lseek(fid, log_d0.offset, SEEK_SET);
                    ::read(fid, d0, setting->part);

                    // 计算delta
                    for (int k = 0; k < setting->part; ++k) {
                        _return.data[index] = Galois::add(Galois::multiply(
                                Galois::subtract(dr[k], d0[k]), gamma),_return.data[index]);
                        index++;
                    }
                } else {
                    index += setting->part;
                }
            }
        }
        ::close(fid);

        _return.code = ResponseCode::SUCCESS;

        delete [] temp;
        delete [] d0;
        delete [] dr;
    }
}

void DDBHW::degrade_read(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    // 采用日志重播的方式恢复dr
    Request req(request);
    req.from_id = request.to_id;
    for (int i = setting->data; i < setting->total; ++i) {
        req.to_id = i;
        Manage::sendDataToServer(&_return, req, setting->clients[i], "READ");
        if(_return.code == ResponseCode::SUCCESS) {
            return;
        }
    }

    // 若奇偶校验服务器宕机，或奇偶校验服务器未存储新数据
    // 采用传统的纠删码解码的方式恢复数据
    CVec2D shards(setting->total);
    BVec presents(setting->total);

    boost::thread_group group;
    Response responses[setting->total];

    // 获取未丢失的数据
    responses[request.to_id].code = ResponseCode::ERROR;
    for (int i = 0; i < setting->total; ++i) {
        if(i != request.to_id) {
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

void DDBHW::mergeData(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    char * temp = new char[setting->part];
    int fid_ifs = ::open((setting->servers[request.to_id]->path + "/user_data.log").c_str(), O_RDONLY, 0777);
    int fid_ofs = ::open((setting->servers[request.to_id]->path + "/user_data.bin").c_str(), O_WRONLY | O_SYNC, 0777);

    for(int i = 0; i < setting->part_num; i++) {
        if(buffer->data_site[i]) {
            ::lseek(fid_ifs, buffer->data_dr_logs[i].back().offset, SEEK_SET);
            ::read(fid_ifs, temp, setting->part);

            ::lseek(fid_ofs, i * setting->part, SEEK_SET);
            ::write(fid_ofs, temp, setting->part);
        }
    }

    ::close(fid_ofs);
    ::close(fid_ifs);
    delete [] temp;
    buffer->clearDataBuffer();
    _return.code = ResponseCode::SUCCESS;
}

void DDBHW::mergeParity(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    char* dr = new char[setting->part];
    char* d0 = new char[setting->part];
    char* pr = new char[setting->part];

    int fid = ::open((setting->servers[request.to_id]->path + "/user_data.log").c_str(), O_RDONLY, 0777);
    for(int i = 0; i < setting->data; i++) {
        int8_t gamma = setting->rs->parityRows[request.to_id - setting->data][i];
        for(int j = 0; j < setting->part_num; j++) {
            if(!buffer->parity_dr_logs[i][j].empty()) {
                // 获取dr
                Log log_dr = buffer->parity_dr_logs[i][j].back();
                ::lseek(fid, log_dr.offset, SEEK_SET);
                ::read(fid, dr, setting->part);

                // 获取d0
                Log log_d0 = buffer->parity_d0_logs[i][j].back();
                ::lseek(fid, log_d0.offset, SEEK_SET);
                ::read(fid, d0, setting->part);

                // 读取p0
                Manage::readData(setting->servers[request.to_id]->path + "/user_data.bin",
                                 pr, log_dr.pid * setting->part, setting->part);

                // 计算pr
                for (int i = 0; i < setting->part; ++i) {
                    pr[i] = Galois::add(Galois::multiply(Galois::subtract(dr[i], d0[i]), gamma), pr[i]);
                }

                // 写入pr
                Manage::writeChunk(setting->servers[request.to_id]->path + "/user_data.bin",
                                   pr, log_dr.pid * setting->part, setting->part);

            }
        }
    }
    ::close(fid);
    delete [] pr;
    delete [] d0;
    delete [] dr;
    buffer->clearParityBuffer();
    _return.code = ResponseCode::SUCCESS;
}
