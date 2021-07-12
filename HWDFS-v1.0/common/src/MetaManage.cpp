#include "MetaManage.h"

MetaManage::MetaManage(const char* config){
    this->manage = new Manage(config);
}

MetaManage::~MetaManage() {
    if(this->manage != NULL) {
        delete this->manage;
        this->manage = NULL;
    }
}

void MetaManage::create(const char* data, unsigned long length) {
    manage->create(data, length);
}
void MetaManage::write(const char* data, unsigned long offset, unsigned long length) {
    if(this->manage->setting->scheme == "REPLICATION" || this->manage->setting->scheme == "REPLICATION_LOG") {
        manage->write(data, offset, length);
    } else {
        TSCNS ts;
        ts.init();

        // 1. 获取更新所涉及的数据块
        MetaRequest meta_request;
        meta_request.offset = offset;
        meta_request.length = length;
        MetaResponse meta_response;
        std::shared_ptr<TTransport> transport = this->manage->setting->master->getOutputProtocol()->getTransport();
        transport->open();
        this->manage->setting->master->getChunkServer(meta_response, meta_request);
        transport->close();

        // 2. 并行更新所有数据块
        Request requests[meta_response.info_list.size()];
        Response responses[meta_response.info_list.size()];
        int64_t temp_length = 0;
        for (int i = 0; i < meta_response.info_list.size(); ++i) {
            requests[i].from_id = -1;
            requests[i].to_id = meta_response.info_list[i].chunk_id;
            requests[i].data = CVec(data + temp_length, data + temp_length + meta_response.info_list[i].length);
            requests[i].scheme = this->manage->setting->scheme;
            requests[i].partId = meta_response.info_list[i].offset / this->manage->setting->part;
            requests[i].offset = offset + temp_length;
            requests[i].length = meta_response.info_list[i].length;
            requests[i].type = DataType::DR;
            temp_length += meta_response.info_list[i].length;
            cout << "Chunk ID: " << requests[i].to_id << endl;
            cout << "Chunk Offset: " << requests[i].offset << endl;
            cout << "Chunk Part ID: " << requests[i].partId << endl;
            cout << "Chunk Length: " << requests[i].length << endl;
        }

        uint64_t begin = ts.rdns();
        boost::thread_group group;
        for (int i = 0; i < meta_response.info_list.size(); ++i) {
            group.add_thread(new boost::thread(Manage::sendDataToServer, &responses[i],
                                               requests[i], this->manage->setting->clients[requests[i].to_id],
                                               "WRITE"));
        }
        uint64_t end = ts.rdns();

        bool flag = true;
        for (int i = 0; i < meta_response.info_list.size(); ++i) {
            if(responses[i].code == ResponseCode::ERROR) {
                flag = false;
                break;
            }
        }

        if(flag) {
            cout << "===============Update Result===============" << endl;
            cout << "update_size:\t" << length << endl;
            cout << "update_offset:\t" << offset << endl;
            cout << "update_scheme:\t" << this->manage->setting->scheme << endl;
            cout << "request_time:\t" << end - begin << endl;
            cout << "===========================================" << endl;
        } else {
            cout << "Server Disconnect......" << endl;
        }
    }
}
void MetaManage::read(char* data, unsigned long offset, unsigned long length) {
    if(this->manage->setting->scheme == "REPLICATION" || this->manage->setting->scheme == "REPLICATION_LOG") {
        manage->read(data, offset, length);
    } else {
        TSCNS ts;
        ts.init();

        // 1. 获取读取所涉及的数据块
        MetaRequest meta_request;
        meta_request.offset = offset;
        meta_request.length = length;
        MetaResponse meta_response;
        std::shared_ptr<TTransport> transport = this->manage->setting->master->getOutputProtocol()->getTransport();
        transport->open();
        this->manage->setting->master->getChunkServer(meta_response, meta_request);
        transport->close();

        // 2. 并行读取所有数据块
        Request requests[meta_response.info_list.size()];
        Response responses[meta_response.info_list.size()];
        int64_t temp_length = 0;
        for (int i = 0; i < meta_response.info_list.size(); ++i) {
            requests[i].from_id = -1;
            requests[i].to_id = meta_response.info_list[i].chunk_id;
            requests[i].data = CVec(data + temp_length, data + temp_length + meta_response.info_list[i].length);
            requests[i].scheme = this->manage->setting->scheme;
            requests[i].partId = meta_response.info_list[i].offset / this->manage->setting->part;
            requests[i].offset = offset + temp_length;
            requests[i].length = meta_response.info_list[i].length;
            requests[i].type = DataType::DR;
            temp_length += meta_response.info_list[i].length;
        }

        uint64_t begin = ts.rdns();
        boost::thread_group group;
        for (int i = 0; i < meta_response.info_list.size(); ++i) {
            group.add_thread(new boost::thread(Manage::sendDataToServer, &responses[i],
                                               requests[i], this->manage->setting->clients[requests[i].to_id],
                                               "READ"));
        }
        uint64_t end = ts.rdns();

        bool flag = true;
        CVec result;
        for (int i = 0; i < meta_response.info_list.size(); ++i) {
            if(responses[i].code == ResponseCode::ERROR) {
                flag = false;
                break;
            } else {
                result.insert(result.end(), responses[i].data.begin(), responses[i].data.end());
            }
        }

        if(flag) {
            memcpy(data, &(result[0]), result.size());
            cout << "===============Read Result===============" << endl;
            cout << "read_size:\t" << length << endl;
            cout << "read_offset:\t" << offset << endl;
            cout << "read_scheme:\t" << this->manage->setting->scheme << endl;
            cout << "request_time:\t" << end - begin << endl;
            cout << "===========================================" << endl;
        } else {
            cout << "Server Disconnect......" << endl;
        }
    }
}
void MetaManage::repair(char* data, unsigned long offset, unsigned long length, DataType::type type) {
    if(this->manage->setting->scheme == "REPLICATION" || this->manage->setting->scheme == "REPLICATION_LOG") {
        manage->repair(data, offset, length, type);
    } else {
        TSCNS ts;
        ts.init();

        // 1. 获取降级读所涉及的数据块
        MetaRequest meta_request;
        meta_request.offset = offset;
        meta_request.length = length;
        MetaResponse meta_response;
        std::shared_ptr<TTransport> transport = this->manage->setting->master->getOutputProtocol()->getTransport();
        transport->open();
        this->manage->setting->master->getChunkServer(meta_response, meta_request);
        transport->close();

        // 2. 并行读取所有数据块
        Request requests[meta_response.info_list.size()];
        Response responses[meta_response.info_list.size()];
        int64_t temp_length = 0;
        for (int i = 0; i < meta_response.info_list.size(); ++i) {
            requests[i].from_id = -1;
            requests[i].to_id = meta_response.info_list[i].chunk_id;
            requests[i].data = CVec(data + temp_length, data + temp_length + meta_response.info_list[i].length);
            requests[i].scheme = this->manage->setting->scheme;
            requests[i].partId = meta_response.info_list[i].offset / this->manage->setting->part;
            requests[i].offset = offset + temp_length;
            requests[i].length = meta_response.info_list[i].length;
            requests[i].type = DataType::DR;
            temp_length += meta_response.info_list[i].length;
        }

        uint64_t begin = ts.rdns();
        boost::thread_group group;
        for (int i = 0; i < meta_response.info_list.size(); ++i) {
            group.add_thread(new boost::thread(Manage::sendDataToServer, &responses[i],
                                               requests[i], this->manage->setting->clients[requests[i].to_id],
                                               "REPAIR"));
        }
        uint64_t end = ts.rdns();

        bool flag = true;
        CVec result;
        for (int i = 0; i < meta_response.info_list.size(); ++i) {
            if(responses[i].code == ResponseCode::ERROR) {
                flag = false;
                break;
            } else {
                result.insert(result.end(), responses[i].data.begin(), responses[i].data.end());
            }
        }

        if(flag) {
            memcpy(data, &(result[0]), result.size());
            cout << "===============Read Result===============" << endl;
            cout << "read_size:\t" << length << endl;
            cout << "read_offset:\t" << offset << endl;
            cout << "read_scheme:\t" << this->manage->setting->scheme << endl;
            cout << "request_time:\t" << end - begin << endl;
            cout << "===========================================" << endl;
        } else {
            cout << "Server Disconnect......" << endl;
        }
    }
}

void MetaManage::merge() {
    manage->merge();
}

void MetaManage::clear() {
    manage->clear();
}

