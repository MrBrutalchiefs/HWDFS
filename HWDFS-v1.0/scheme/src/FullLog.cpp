#include "FullLog.h"

void FullLog::writeData(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    //读取d(r-1)
    int pn = request.length / setting->part;
    CVec d0;
    char * temp = new char[setting->part];
    int fid_dr = ::open((setting->servers[request.to_id]->path + "/user_data.log").c_str(), O_RDONLY, 0777);
    int fid_d0 = ::open((setting->servers[request.to_id]->path + "/user_data.bin").c_str(), O_RDONLY, 0777);
    for (int i = request.partId; i < request.partId + pn; ++i) {
        if(!buffer->data_dr_logs[i].empty()) {
            ::lseek(fid_dr, buffer->data_dr_logs[i].back().offset, SEEK_SET);
            ::read(fid_dr, temp, setting->part);
        } else {
            ::lseek(fid_d0, i * setting->part, SEEK_SET);
            ::read(fid_d0, temp, setting->part);
        }
        d0.insert(d0.end(), temp, temp + setting->part);
    }
    ::close(fid_dr);
    ::close(fid_d0);
    delete [] temp;

    //计算增量(d(r)-d(r-1)) * gamma
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
            reqs[i].data[j] = Galois::multiply(Galois::subtract(request.data[j], d0[j]), gamma);
        }
    }

    // 更新数据
    int64_t curr_offset = 0;
    char* update_data = new char[request.length];
    memcpy(update_data, &(request.data[0]), request.length);
    Response responses[setting->parity];
    boost::thread_group group;
    group.add_thread(new boost::thread(Manage::writeLog,
                                       setting->servers[request.to_id]->path + "/user_data.log",
                                       update_data, request.length, &curr_offset));
    for (int i = 0; i < setting->parity; ++i) {
        group.add_thread(new boost::thread(Manage::sendDataToServer, &responses[i], reqs[i],
                                           setting->clients[reqs[i].to_id], "WRITE"));
    }
    group.join_all();
    delete [] update_data;

    // 元数据记录
    TSCNS ts;
    ts.init();
    for (int i = request.partId; i < request.partId + pn; ++i) {
        buffer->data_dr_logs[i].push_back(Log(i, curr_offset, ts.rdns()));
        curr_offset += setting->part;
    }
    _return.code = ResponseCode::SUCCESS;
}

void FullLog::writeParity(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    // 将更新数据追加到日志中
    char* temp = new char[request.length];
    memcpy(temp, &(request.data[0]), request.length);
    int64_t curr_offset = 0;
    Manage::writeLog(setting->servers[request.to_id]->path + "/user_data.log",
                     temp, request.length, &curr_offset);
    delete [] temp;

    //临时存放
    TSCNS ts;
    ts.init();
    int64_t pn = request.length / setting->part;
    for (int i = request.partId; i < request.partId + pn; ++i) {
        buffer->parity_dr_logs[request.to_id][i].push_back(Log(i, curr_offset, ts.rdns()));
        curr_offset += setting->part;
    }
    _return.code = ResponseCode::SUCCESS;
}

void FullLog::readData(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    if(request.type == comm::DataType::D0) {
        char * temp = new char[request.length];
        Manage::readData(setting->servers[request.to_id]->path + "/user_data.bin",
                         temp, request.partId * setting->part, request.length);
        _return.data = CVec(temp, temp + request.length);
        delete [] temp;
        _return.code = ResponseCode::SUCCESS;
    } else if(request.type == comm::DataType::DR) {
        int pn = request.length / setting->part;
        char * temp = new char[setting->part];

        int fid_dr = ::open((setting->servers[request.to_id]->path + "/user_data.log").c_str(), O_RDONLY, 0777);
        int fid_d0 = ::open((setting->servers[request.to_id]->path + "/user_data.bin").c_str(), O_RDONLY, 0777);

        for (int i = request.partId; i < request.partId + pn; ++i) {
            int size = buffer->data_dr_logs[i].size();
            if(size != 0) {
                ::lseek(fid_dr, buffer->data_dr_logs[i][size - 1].offset, SEEK_SET);
                ::read(fid_dr, temp, setting->part);
            } else {
                ::lseek(fid_d0, i * setting->part, SEEK_SET);
                ::read(fid_d0, temp, setting->part);
            }
            _return.data.insert(_return.data.end(), temp, temp + setting->part);
        }
        ::close(fid_dr);
        ::close(fid_d0);
        delete [] temp;
        _return.code = ResponseCode::SUCCESS;
    }
}

void FullLog::readParity(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
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

void FullLog::degrade_read(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    //不论是原始数据还是最新数据，均采用传统纠删码更新方式
    if(request.type == comm::DataType::D0) {
        // 采用传统的纠删码解码的方式回复原始数据d0
        CVec2D shards(setting->total);
        BVec presents(setting->total);

        //封装Request
        Request req(request);
        req.from_id = request.to_id;
        boost::thread_group group;
        Response responses[setting->total];
        responses[request.to_id].code = ResponseCode::ERROR;
        for (int i = 0; i < setting->total; ++i) {
            if(i != request.to_id) {
                req.to_id = i;
                if(i >= setting->data) {
                    req.type = comm::DataType::P0;
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

        setting->rs->decodeMissing(shards, presents, index, request.length);

        _return.data = CVec(shards[request.to_id].begin() + index,
                            shards[request.to_id].begin() + index + request.length);
        _return.code = ResponseCode::SUCCESS;
        return;
    } else if(request.type == comm::DataType::DR) {
        // 采用传统的纠删码解码的方式回复原始数据d0
        CVec2D shards(setting->total);
        BVec presents(setting->total);

        boost::thread_group group;
        Response responses[setting->total];
        //封装Request
        Request req(request);
        req.from_id = request.to_id;
        responses[request.to_id].code = ResponseCode::ERROR;
        for (int i = 0; i < setting->total; ++i) {
            if (i != request.to_id) {
                req.to_id = i;
                if (i >= setting->data) {
                    req.type = comm::DataType::PR;
                }
                group.add_thread(new boost::thread(Manage::sendDataToServer, &responses[i], req,
                                                   setting->clients[i], "READ"));
            }
        }
        group.join_all();

        int64_t index = request.partId * setting->part;
        for (int i = 0; i < setting->total; ++i) {
            shards[i] = CVec(setting->size);
            if (responses[i].code == ResponseCode::SUCCESS) {
                for (int j = 0; j < request.length; ++j) {
                    shards[i][j + index] = responses[i].data[j];
                }
                presents[i] = true;
            } else {
                presents[i] = false;
            }
        }

        setting->rs->decodeMissing(shards, presents, index, request.length);

        _return.data = CVec(shards[request.to_id].begin() + index,
                            shards[request.to_id].begin() + index + request.length);
        _return.code = ResponseCode::SUCCESS;
    }
}

void FullLog::mergeData(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    char * temp = new char[setting->part];

    int fid_ifs = ::open((setting->servers[request.to_id]->path + "/user_data.log").c_str(), O_RDONLY, 0777);
    int fid_ofs = ::open((setting->servers[request.to_id]->path + "/user_data.bin").c_str(), O_WRONLY | O_SYNC, 0777);

    for (int i = 0; i < setting->part_num; i++) {
        if(!buffer->data_dr_logs[i].empty()) {
            Log log_temp = buffer->data_dr_logs[i].back();

            // 读取日志
            ::lseek(fid_ifs, log_temp.offset, SEEK_SET);
            ::read(fid_ifs, temp, setting->part);

            // 更新数据
            ::lseek(fid_ofs, log_temp.pid * setting->part, SEEK_SET);
            ::write(fid_ofs, temp, setting->part);
        }
    }
    ::close(fid_ifs);
    ::close(fid_ofs);
    delete [] temp;
    buffer->clearDataBuffer();
    _return.code = ResponseCode::SUCCESS;
}

void FullLog::mergeParity(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    char* temp = new char[setting->part];
    char* delta = new char[setting->part];

    int fid = ::open((setting->servers[request.to_id]->path + "/user_data.log").c_str(), O_RDONLY, 0777);

    for(int i = 0; i < setting->part_num; i++) {
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
    buffer->clearDataBuffer();
    _return.code = ResponseCode::SUCCESS;
}

