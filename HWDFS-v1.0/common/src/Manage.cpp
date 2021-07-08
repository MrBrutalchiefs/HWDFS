#include "Manage.h"

Manage::Manage(const char *config) {
    this->setting = RSConfiguration::load(config);
}

Manage::~Manage() {
    if(this->setting != NULL) {
        delete this->setting;
        this->setting = NULL;
    }
}

void Manage::create(const char *data, unsigned long length) {
    if(this->setting->scheme == "REPLICATION" || this->setting->scheme == "REPLICATION_LOG") { // 采用备份
        CVec new_data(data, data + length);

        Response response[this->setting->parity + 1];
        boost::thread_group group;
        TSCNS ts;
        ts.init();

        //封装Request
        Request request;
        request.from_id = -1;
        request.data = CVec(data, data + length);
        request.scheme = this->setting->scheme;
        request.type = DataType::D0;
        request.partId = 0;
        request.offset = 0;
        request.length = length;

        // 并行发送请求
        uint64_t begin = ts.rdns();
        for (int i = 0; i < this->setting->parity + 1; ++i) {
            request.to_id = this->setting->data - 1 + i;
            group.add_thread(new boost::thread(Manage::sendDataToServer, &response[i], request,
                                               this->setting->clients[request.to_id], "CREATE"));
        }
        group.join_all();
        uint64_t end = ts.rdns();

        bool flag = true;
        for (int i = 0; i < this->setting->parity + 1; ++i) {
            if(response[i].code == ResponseCode::ERROR) {
                flag = false;
                break;
            }
        }

        if(flag) {
            cout << "===============REPLICATION===============" << endl;
            cout << "replication_num:\t" << this->setting->parity + 1 << endl;
            cout << "replication_size:\t" << length << endl;
            cout << "request_time:\t\t" << end - begin << endl;
            cout << "=========================================" << endl;
        } else {
            cout << "Server Disconnect......" << endl;
        }
    } else { // 采用纠删码
        TSCNS ts;
        ts.init();

        // 初始化数据块
        CVec2D shards(this->setting->total);
        for (int i = 0; i < this->setting->total; ++i) {
            if(i >= this->setting->data) {
                shards[i] = CVec(this->setting->size);
                continue;
            }
            shards[i] = CVec(data + i * this->setting->size,
                             data + (i + 1) * this->setting->size);
        }

        // 编码
        uint64_t encode_begin = ts.rdns();
        this->setting->rs->encodeParity(shards, 0, this->setting->size);
        uint64_t encode_end = ts.rdns();

        // 封装Request
        Request request;
        request.from_id = -1;
        request.scheme = this->setting->scheme;
        request.partId = 0;
        request.offset = 0;
        request.length = this->setting->size;
        request.type = DataType::D0;

        // 向服务器发送数据
        Response response[this->setting->total];
        boost::thread_group group;
        uint64_t begin = ts.rdns();
        for (int i = 0; i < this->setting->total; ++i) {
            request.to_id = i;
            request.data = shards[i];
            group.add_thread(new boost::thread(Manage::sendDataToServer,&response[i], request,
                                               this->setting->clients[i], "CREATE"));
        }
        group.join_all();
        uint64_t end = ts.rdns();

        bool flag = true;
        for (int i = 0; i < this->setting->total; ++i) {
            if(response[i].code == ResponseCode::ERROR) {
                flag = false;
                break;
            }
        }

        if(flag) {
            cout << "===============Code Result===============" << endl;
            cout << "data_num:\t\t" << this->setting->data << endl;
            cout << "parity_num:\t\t" << this->setting->parity << endl;
            cout << "chunk_size:\t\t" << this->setting->size << endl;
            cout << "encode_time:\t" << encode_end - encode_begin << endl;
            cout << "request_time:\t" << end - begin << endl;
            cout << "=========================================" << endl;
        } else {
            cout << "Server Disconnect......" << endl;
        }
    }
}

void Manage::write(const char *data, unsigned long offset, unsigned long length) {
    TSCNS ts;
    ts.init();

    if(this->setting->scheme == "REPLICATION" || this->setting->scheme == "REPLICATION_LOG") {
        Response response;

        // Request封装
        Request request;
        request.from_id = -1;
        request.to_id = this->setting->data - 1;
        request.data = CVec(data, data + length);
        request.scheme = this->setting->scheme;
        request.partId = offset / this->setting->part;
        request.offset = offset;
        request.length = length;
        request.type = DataType::DR;

        uint64_t begin = ts.rdns();
        sendDataToServer(&response, request, this->setting->clients[request.to_id], "WRITE");
        uint64_t end = ts.rdns();

        if(response.code == ResponseCode::SUCCESS) {
            cout << "===============Update Result===============" << endl;
            cout << "update_size:\t" << length << endl;
            cout << "update_offset:\t" << offset << endl;
            cout << "update_scheme:\t" << this->setting->scheme << endl;
            cout << "request_time:\t" << end - begin << endl;
            cout << "===========================================" << endl;
        } else {
            cout << "Server Disconnect......" << endl;
        }
    } else {
        Response response;

        // Request封装
        Request request;
        request.from_id = -1;
        request.to_id = offset / this->setting->size;
        request.data = CVec(data, data + length);
        request.scheme = this->setting->scheme;
        request.partId = (offset % this->setting->size) / this->setting->part;
        request.offset = offset;
        request.length = length;
        request.type = DataType::DR;

        // 发送请求
        uint64_t begin = ts.rdns();
        sendDataToServer(&response, request, this->setting->clients[request.to_id], "WRITE");
        uint64_t end = ts.rdns();

        if(response.code == ResponseCode::SUCCESS) {
            cout << "===============Update Result===============" << endl;
            cout << "chunk_id:\t\t" << request.to_id << endl;
            cout << "update_size:\t" << length << endl;
            cout << "update_offset:\t" << request.partId << endl;
            cout << "update_scheme:\t" << this->setting->scheme << endl;
            cout << "request_time:\t" << end - begin << endl;
            cout << "===========================================" << endl;
        }
    }
}

void Manage::read(char *data, unsigned long offset, unsigned long length) {
    if(this->setting->scheme == "REPLICATION" || this->setting->scheme == "REPLICATION_LOG") {
        Response response;
        TSCNS ts;
        ts.init();

        // Request封装
        Request request;
        request.from_id = -1;
        request.to_id = this->setting->data - 1;
        request.scheme = this->setting->scheme;
        request.partId = offset / this->setting->part;
        request.offset = offset;
        request.length = length;
        request.type = DataType::DR;

        uint64_t begin = ts.rdns();
        sendDataToServer(&response, request, this->setting->clients[request.to_id], "READ");
        uint64_t end = ts.rdns();

        if(response.code == ResponseCode::SUCCESS) {
            memcpy(data, &(response.data[0]), response.data.size());
            cout << "===============Read Result===============" << endl;
            cout << "read_size:\t" << length << endl;
            cout << "read_offset:\t" << offset << endl;
            cout << "read_scheme:\t" << this->setting->scheme << endl;
            cout << "request_time:\t" << end - begin << endl;
            cout << "=========================================" << endl;
        } else {
            cout << "Server Disconnect......" << endl;
        }
    } else {
        Response response;
        TSCNS ts;
        ts.init();

        // Request封装
        Request request;
        request.from_id = -1;
        request.to_id = offset / this->setting->size;
        request.scheme = this->setting->scheme;
        request.partId = (offset % this->setting->size) / this->setting->part;
        request.offset = offset;
        request.length = length;
        request.type = DataType::DR;


        uint64_t begin = ts.rdns();
        sendDataToServer(&response, request, this->setting->clients[request.to_id],"READ");
        uint64_t end = ts.rdns();

        if(response.code == ResponseCode::SUCCESS) {
            memcpy(data, &(response.data[0]), response.data.size());
            cout << "===============Read Result===============" << endl;
            cout << "chunk_id:\t\t" << request.to_id << endl;
            cout << "read_size:\t" << length << endl;
            cout << "read_offset:\t" << request.partId << endl;
            cout << "read_scheme:\t" << this->setting->scheme << endl;
            cout << "request_time:\t" << end - begin << endl;
            cout << "=========================================" << endl;
        }
    }
}

void Manage::repair(char *data, unsigned long offset, unsigned long length, DataType::type type) {
    if(this->setting->scheme == "REPLICATION" || this->setting->scheme == "REPLICATION_LOG") {
        Response response;
        TSCNS ts;
        ts.init();

        // Request封装
        Request request;
        request.from_id = -1;
        request.to_id = this->setting->data - 1;
        request.scheme = this->setting->scheme;
        request.partId = offset / this->setting->part;
        request.offset = offset;
        request.length = length;
        request.type = type;

        uint64_t begin = ts.rdns();
        sendDataToServer(&response, request, this->setting->clients[request.to_id], "REPAIR");
        uint64_t end = ts.rdns();

        if(response.code == ResponseCode::SUCCESS) {
            memcpy(data, &(response.data[0]), response.data.size());
            cout << "===============Read Result===============" << endl;
            cout << "read_size:\t" << length  << endl;
            cout << "read_offset:\t" << offset << endl;
            cout << "read_scheme:\t" << this->setting->scheme << endl;
            cout << "request_time:\t" << end - begin << endl;
            cout << "=========================================" << endl;
        } else {
            cout << "Server Disconnect......" << endl;
        }
    } else {
        Response response;
        TSCNS ts;
        ts.init();

        // Request封装
        Request request;
        request.from_id = -1;
        request.to_id = offset / this->setting->size;
        request.scheme = this->setting->scheme;
        request.partId = (offset % this->setting->size) / this->setting->part;
        request.offset = offset;
        request.length = length;
        request.type = type;

        uint64_t begin = ts.rdns();
        sendDataToServer(&response, request, this->setting->clients[request.to_id], "REPAIR");
        uint64_t end = ts.rdns();

        if(response.code == ResponseCode::SUCCESS) {
            memcpy(data, &(response.data[0]), response.data.size());
            cout << "===============Read Result===============" << endl;
            cout << "chunk_id:\t\t" << request.to_id << endl;
            cout << "read_size:\t" << length  << endl;
            cout << "read_offset:\t" << request.partId << endl;
            cout << "read_scheme:\t" << this->setting->scheme << endl;
            cout << "request_time:\t" << end - begin << endl;
            cout << "=========================================" << endl;
        }
    }
}

void Manage::merge() {
    Response response[setting->total];
    boost::thread_group group;

    // Request封装
    Request request;
    request.from_id = -1;
    request.scheme = this->setting->scheme;
    for (int i = 0; i < setting->total; ++i) {
        request.to_id = i;
        group.add_thread(new boost::thread(sendDataToServer,&response[i],
                                           request,this->setting->clients[i], "MERGE"));
    }
    group.join_all();

    bool flag = true;
    for (int i = 0; i < setting->total; ++i) {
        if(response[i].code == ResponseCode::ERROR) {
            flag = false;
        }
    }
    if(flag) {
        cout << "合并成功！" << endl;
    } else {
        cout << "合并失败！" << endl;
    }

}

void Manage::clear() {
    if(setting->scheme == "REPLICATION" || setting->scheme == "REPLICATION_LOG") {
        Response response[setting->parity + 1];
        boost::thread_group group;

        // Request封装
        Request request;
        request.from_id = -1;
        request.scheme = this->setting->scheme;
        for (int i = setting->data - 1; i < setting->total; ++i) {
            request.to_id = i;
            group.add_thread(new boost::thread(sendDataToServer,&response[i],
                                               request,this->setting->clients[i], "DELETE"));
        }
        group.join_all();

        bool flag = true;
        for (int i = 0; i < setting->parity + 1; ++i) {
            if(response[i].code == ResponseCode::ERROR) {
                flag = false;
            }
        }
        if(flag) {
            cout << "服务器数据清空成功！" << endl;
        } else {
            cout << "服务器数据清空失败！" << endl;
        }
    } else {
        Response response[setting->total];
        boost::thread_group group;

        // Request封装
        Request request;
        request.from_id = -1;
        request.scheme = this->setting->scheme;
        for (int i = 0; i < setting->total; ++i) {
            request.to_id = i;
            group.add_thread(new boost::thread(sendDataToServer,&response[i],
                                               request,this->setting->clients[i], "DELETE"));
        }
        group.join_all();

        bool flag = true;
        for (int i = 0; i < setting->total; ++i) {
            if(response[i].code == ResponseCode::ERROR) {
                flag = false;
            }
        }
        if(flag) {
            cout << "服务器数据清空成功！" << endl;
        } else {
            cout << "服务器数据清空失败！" << endl;
        }
    }
}

void Manage::sendDataToServer(Response* response, Request request, StoreServiceClient* client, string type) {
    std::shared_ptr<TTransport> transport = client->getOutputProtocol()->getTransport();
    transport->open();
    if(type == "CREATE") {
        client->create(*response, request);
    } else if(type == "WRITE") {
        client->write(*response, request);
    } else if(type == "READ") {
        client->read(*response, request);
    } else if(type == "REPAIR") {
        client->degrade_read(*response, request);
    } else if(type == "MERGE") {
        client->merge(*response, request);
    } else if(type == "DELETE") {
        client->clear(*response, request);
    }

    transport->close();
}

void Manage::writeLog(string path, char* data, int64_t length, int64_t *curr_offset) {
    int fid = ::open(path.c_str(), O_CREAT | O_WRONLY | O_APPEND | O_SYNC, 0777);
    ::write(fid, data, length);
    *curr_offset = ::lseek(fid, 0, SEEK_CUR) - length;
    ::close(fid);
}

void Manage::writeChunk(string path, char* data, int64_t offset, int64_t length) {
    int fid = ::open(path.c_str(), O_CREAT | O_WRONLY | O_SYNC, 0777);
    ::lseek(fid, offset, SEEK_SET);
    ::write(fid, data, length);
    ::close(fid);
}

void Manage::readData(string path, char* data, int64_t offset, int64_t length) {
    int fid = ::open(path.c_str(), O_RDONLY, 0777);
    ::lseek(fid, offset, SEEK_SET);
    ::read(fid, data, length);
    ::close(fid);
}


