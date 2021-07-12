#include "Replication_Log.h"

void ReplicationLog::writeData(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
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

void ReplicationLog::writeParity(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
    char * temp = new char[request.length];
    memcpy(temp, &(request.data[0]), request.length);

    int64_t curr_offset = 0;
    Manage::writeLog(setting->servers[request.to_id]->path + "/user_data.log",
                     temp, request.length, &curr_offset);
    delete [] temp;
    int pn = request.length / setting->part;
    TSCNS ts;
    ts.init();
    for (int i = request.partId; i < request.partId + pn; ++i) {
        buffer->parity_dr_logs[request.to_id][i].push_back(Log(i, curr_offset, ts.rdns()));
        curr_offset += setting->part;
    }
}

void ReplicationLog::readData(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
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

        int fid_dr = ::open((setting->servers[request.to_id]->path + "/user_data.log").c_str(), O_RDONLY);
        int fid_d0 = ::open((setting->servers[request.to_id]->path + "/user_data.bin").c_str(), O_RDONLY);

        for (int i = request.partId; i < request.partId + pn; ++i) {
            if(!buffer->data_dr_logs[i].empty()) {
                ::lseek(fid_dr, buffer->data_dr_logs[i].back().offset, SEEK_SET);
                ::read(fid_dr, temp, setting->part);
            } else {
                ::lseek(fid_d0, i * setting->part, SEEK_SET);
                ::read(fid_dr, temp, setting->part);
            }
            _return.data.insert(_return.data.end(), temp, temp + setting->part);
        }
        ::close(fid_dr);
        ::close(fid_d0);
        delete [] temp;
        _return.code = ResponseCode::SUCCESS;
    }
}

void ReplicationLog::readParity(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
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

        int fid = ::open((setting->servers[request.to_id]->path + "/user_data.bin").c_str(), O_RDONLY);
        int fid_log = ::open((setting->servers[request.to_id]->path + "/user_data.log").c_str(), O_RDONLY);

        for (int i = request.partId; i < request.partId + pn; ++i) {
            if(!buffer->parity_dr_logs[request.to_id][i].empty()) {
                ::lseek(fid_log, buffer->parity_dr_logs[request.to_id][i].back().offset, SEEK_SET);
                ::read(fid_log, temp, setting->part);
            } else {
                ::lseek(fid, i * setting->part, SEEK_SET);
                ::read(fid, temp, setting->part);
            }
            _return.data.insert(_return.data.end(), temp, temp + setting->part);
        }
        ::close(fid);
        ::close(fid_log);
        delete [] temp;
        _return.code = ResponseCode::SUCCESS;
        return;
    }
}

void ReplicationLog::degrade_read(Response &_return, const Request &request, Buffer* buffer, Setting* setting) {
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

void ReplicationLog::mergeData(Response &_return, const Request& request, Buffer* buffer, Setting* setting) {
    char * temp = new char[setting->part];

    int fid_ifs = ::open((setting->servers[request.to_id]->path + "/user_data.log").c_str(), O_RDONLY);
    int fid_ofs = ::open((setting->servers[request.to_id]->path + "/user_data.bin").c_str(), O_WRONLY | O_SYNC, 0777);

    for (int i = 0; i < setting->part_num; i ++) {
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

void ReplicationLog::mergeParity(Response &_return, const Request& request, Buffer* buffer, Setting* setting) {
    char * temp = new char[setting->part];

    int fid_ifs = ::open((setting->servers[request.to_id]->path + "/user_data.log").c_str(), O_RDONLY);
    int fid_ofs = ::open((setting->servers[request.to_id]->path + "/user_data.bin").c_str(), O_WRONLY | O_SYNC, 0777);

    for (int i = 0; i < setting->part_num; i++) {
        if(!buffer->parity_dr_logs[request.to_id][i].empty()) {
            Log log_temp = buffer->parity_dr_logs[request.to_id][i].back();

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
    buffer->clearParityBuffer();
    _return.code = ResponseCode::SUCCESS;
}