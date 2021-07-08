#include "PWDR.h"

void PWDR::writeData(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    // 并行更新数据
    int64_t curr_offset = 0;
    Response responses[setting->parity];
    Request req(request);
    req.from_id = request.to_id;
    char* temp = new char[request.length];
    memcpy(temp, &(request.data[0]), request.length);
    boost::thread_group group;
    group.add_thread(new boost::thread(Manage::writeLog,
                                       setting->servers[request.to_id]->path + "/user_data.log",
                                       temp, request.length, &curr_offset));
    for (int i = 0; i < setting->parity; ++i) {
        req.to_id = i + setting->data;
        group.add_thread(new boost::thread(Manage::sendDataToServer, &responses[i],
                                           req, setting->clients[req.to_id], "WRITE"));
    }
    group.join_all();
    delete [] temp;

    TSCNS ts;
    ts.init();
    int pn = request.length / setting->part;
    for (int i = request.partId; i < request.partId + pn; ++i) {
        buffer->data_dr_logs[i].push_back(Log(i, curr_offset, ts.rdns()));
        curr_offset += setting->part;
    }
    _return.code = ResponseCode::SUCCESS;
}

void PWDR::writeParity(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    // 判断当前服务器编号
    int64_t pn = request.length / setting->part;
    char* temp = new char[request.length];
    memcpy(temp, &(request.data[0]), request.length);
    
    // 将更新数据追加到日志中
    int64_t curr_offset = 0;
    Manage::writeLog(setting->servers[request.to_id]->path + "/user_data.log",
                     temp, request.length, &curr_offset);
    delete [] temp;

    // 临时存放
    TSCNS ts;
    ts.init();
    for (int i = request.partId; i < request.partId + pn; ++i) {
        buffer->parity_dr_logs[request.from_id][i].push_back(Log(request.from_id, i, curr_offset, ts.rdns(), 0));
        curr_offset += setting->part;
    }
    _return.code = ResponseCode::SUCCESS;
}

void PWDR::readData(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
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

void PWDR::readParity(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
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
                ::close(fid);
                delete [] temp;
                _return.code = ResponseCode::ERROR;
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
        // 读取d0
        Request req(request);
        req.from_id = request.to_id;
        req.to_id = request.from_id;
        req.type = comm::DataType::D0;
        Response response;
        Manage::sendDataToServer(&response, req, setting->clients[req.to_id], "READ");

        // 读取dr，并计算数据增量
        _return.data = CVec(request.length);
        char* dr = new char[setting->part];
        int64_t index = 0;
        int pn = request.length / setting->part;
        int fid = ::open((setting->servers[request.to_id]->path + "/user_data.log").c_str(), O_RDONLY, 0777);
        for (int i = request.partId; i < request.partId + pn; ++i) {
            if(!buffer->parity_dr_logs[request.from_id][i].empty()) {
                Log log_temp = buffer->parity_dr_logs[request.from_id][i].back();

                ::lseek(fid, log_temp.offset, SEEK_SET);
                ::read(fid, dr, setting->part);

                for (int j = 0; j < setting->part; ++j) {
                    _return.data[index] = Galois::subtract(dr[j], response.data[index]);
                    index++;
                }
            } else {
                index += setting->part;
            }
        }
        delete [] dr;
        ::close(fid);

        // 读取p0
        char* p0 = new char[request.length];
        Manage::readData(setting->servers[request.to_id]->path + "/user_data.bin",
                         p0, request.partId * setting->part, request.length);

        // 计算pr
        int8_t gamma = setting->rs->parityRows[request.to_id][request.from_id];
        for (int i = 0; i < request.length; ++i) {
            _return.data[i] = Galois::add(Galois::multiply(_return.data[i], gamma), p0[i]);
        }
        _return.code = ResponseCode::SUCCESS;
        delete [] p0;
    }
}

void PWDR::degrade_read(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    if(request.type == comm::DataType::D0) {
        // 采用传统的纠删码解码的方式恢复数据
        CVec2D shards(setting->total);
        BVec presents(setting->total);

        boost::thread_group group;
        Response responses[setting->total];

        // 获取未丢失的数据
        Request req(request);
        req.from_id = request.to_id;
        req.type = comm::DataType::D0;
        responses[request.to_id].code = ResponseCode::ERROR;
        for (int i = 0; i < setting->total; ++i) {
            if(i != request.to_id) {
                //封装Request
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

        // 解码
        setting->rs->decodeMissing(shards, presents, index, request.length);

        _return.data = CVec(shards[request.to_id].begin() + index,
                            shards[request.to_id].begin() + index + request.length);
        _return.code = ResponseCode::SUCCESS;
    } else if(request.type == comm::DataType::DR) {
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
}

void PWDR::mergeData(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
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

void PWDR::mergeParity(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    char* dr = new char[setting->part];
    int fid = ::open((setting->servers[request.to_id]->path + "/user_data.log").c_str(), O_RDONLY, 0777);
    for (int i = 0; i < setting->data; i++) {
        int8_t gamma = setting->rs->parityRows[request.to_id - setting->data][i];

        // 获取日志范围
        int min_pid = -1;
        int max_pid = -1;
        for (int j = 0; j < setting->part_num; j++) {
            if(min_pid == -1) {
                if(j == 0 && !buffer->parity_dr_logs[i][j].empty()) {
                    min_pid = j;
                } else if(j > 0 && buffer->parity_dr_logs[i][j - 1].empty()
                                && !buffer->parity_dr_logs[i][j].empty()) {
                    min_pid = j;
                }
            }

            if(max_pid == -1) {
                if(j < setting->part_num - 1
                        && !buffer->parity_dr_logs[i][j].empty()
                        && buffer->parity_dr_logs[i][j + 1].empty()) {
                    max_pid = j;
                } else if(j == setting->part_num - 1) {
                    max_pid = j;
                }
            }
        }

        // 远程读取d0
        if(min_pid != -1 && max_pid != -1) {
            Response response;
            Request req;
            req.from_id = request.to_id;
            req.to_id = i;
            req.scheme = request.scheme;
            req.partId = min_pid;
            req.length = (max_pid - min_pid + 1) * setting->part;
            req.type = comm::DataType::D0;
            Manage::sendDataToServer(&response, req, setting->clients[req.to_id], "READ");

            //获取dr，并计算增量
            CVec delta(req.length);
            int64_t index = 0;
            for (int j = min_pid; j <= max_pid; ++j) {
                if(!buffer->parity_dr_logs[i][j].empty()) {
                    // 获取dr
                    Log log_dr = buffer->parity_dr_logs[i][j].back();
                    ::lseek(fid, log_dr.offset, SEEK_SET);
                    ::read(fid, dr, setting->part);

                    for (int k = 0; k < setting->part; ++k) {
                        delta[index] = Galois::subtract(dr[j], response.data[index]);
                        index++;
                    }
                } else {
                    index += setting->part;
                }
            }

            // 读取p0
            char* pr = new char[req.length];
            Manage::readData(setting->servers[request.to_id]->path + "/user_data.bin",
                             pr, min_pid * setting->part, req.length);

            // 计算pr，并写入磁盘
            for (int i = 0; i < req.length; ++i) {
                pr[i] = Galois::add(Galois::multiply(delta[i], gamma), pr[i]);
            }
            Manage::writeChunk(setting->servers[request.to_id]->path + "/user_data.bin",
                               pr, min_pid * setting->part, req.length);
            delete [] pr;
        }
    }
    ::close(fid);
    delete [] dr;
    buffer->clearParityBuffer();
    _return.code = ResponseCode::SUCCESS;
}
