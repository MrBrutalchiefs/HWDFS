#pragma once
#include <tsl/robin_map.h>

using namespace tsl;

class Buffer {
public:
    int total;
    int part_num;

    vector<Log>* data_dr_logs = NULL;
    robin_map<std::string, bool> data_is_update;
    bool* data_site = NULL;
    vector<Log>** parity_dr_logs = NULL;
    vector<Log>** parity_d0_logs = NULL;

    Buffer(const Setting& setting) {
        this->total = setting.total;
        this->part_num = setting.part_num;
        this->data_dr_logs = new vector<Log>[part_num];
        this->data_site = new bool[part_num];

        parity_d0_logs = new vector<Log>*[total];
        parity_dr_logs = new vector<Log>*[total];
        for (int i = 0; i < total; ++i) {
            parity_dr_logs[i] = new vector<Log>[part_num];
            parity_d0_logs[i] = new vector<Log>[part_num];
        }
    }

    ~Buffer() {
        if(data_dr_logs != NULL) delete [] data_dr_logs;
        if(data_site != NULL) delete [] data_site;
        if(parity_dr_logs != NULL) {
            for (int i = 0; i < total; ++i) {
                delete [] parity_dr_logs[i];
            }
            delete [] parity_dr_logs;
        }
        if(parity_d0_logs != NULL) {
            for (int i = 0; i < total; ++i) {
                delete [] parity_d0_logs[i];
            }
            delete [] parity_d0_logs;
        }
    }

    void clearDataBuffer() {
        if(data_dr_logs != NULL) delete [] data_dr_logs;
        if(data_site != NULL) delete [] data_site;
        data_is_update.clear();
        this->data_dr_logs = new vector<Log>[part_num];
        this->data_site = new bool[part_num];
    }

    void clearParityBuffer() {
        if(parity_dr_logs != NULL) {
            for (int i = 0; i < total; ++i) {
                delete [] parity_dr_logs[i];
            }
            delete [] parity_dr_logs;
        }
        if(parity_d0_logs != NULL) {
            for (int i = 0; i < total; ++i) {
                delete [] parity_d0_logs[i];
            }
            delete [] parity_d0_logs;
        }

        parity_d0_logs = new vector<Log>*[total];
        parity_dr_logs = new vector<Log>*[total];
        for (int i = 0; i < total; ++i) {
            parity_dr_logs[i] = new vector<Log>[part_num];
            parity_d0_logs[i] = new vector<Log>[part_num];
        }
    }
};
