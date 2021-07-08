#include <iostream>
#include <fstream>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "Manage.h"

class Trace {
public:
    string type;
    int64_t offset;
    int64_t length;

    Trace() {}

    Trace(int64_t o, int64_t l) {
        this->offset = o;
        this->length = l;
    }

    Trace(string t, int64_t o, int64_t l) {
        this->type = t;
        this->offset = o;
        this->length = l;
    }
};

void createNewData(char* data, int64_t length);
void createUpdateData(char* data, int64_t length, string path);

void create();

void update_sample();
void read_sample();
void degard_read_sample();

void update_iops(int rounds, int num, int64_t update_size);
void read_iops(int rounds, int num, int64_t update_size);
void degard_read_iops(int rounds, int num, int64_t update_size);

void update_traces(string path);
void read_traces(string path);
void hybrid_traces(string path);
void degard_read_traces(string path);

Manage* manage = NULL;
string sample_path = "/home/data/traces/trace_sample.csv";
string traces_path = "/home/data/traces";
string update_sample_path = "/home/data/update/sample";
string update_iops_path = "/home/data/update/iops";
string update_trace_path = "/home/data/update/trace";
string update_repair_path = "/home/data/update/repair";
string update_hybrid_path = "/home/data/update/hybrid";
string config_path = "/home/data";

int main(int argc, char* argv[]) {
    if(string(argv[1]) == "-h") {
        cout << "-c: " << "Create user data" << endl;
        cout << "-d: " << "Delete user data" << endl;
        cout << "-s: " << "Replay trace in sample" << endl;
        cout << "\twrite:\t" << "Replay write trace in sample" << endl;
        cout << "\tread:\t" << "Replay read trace in sample" << endl;
        cout << "\trepair:\t" << "Replay repair trace in sample" << endl;
        cout << "-i: " << "Replay trace in iops" << endl;
        cout << "\twrite [a] [b] [c]:\t" << "Write a rounds, write b times each round, write b each time" << endl;
        cout << "\tread [a] [b] [b]:\t" << "Read b times each round, read b each time" << endl;
        cout << "\trepair [a] [b] [c]:\t" << "Repair b times each round, Repair b each time" << endl;
        cout << "-t: " << "Replay MSR trace" << endl;
        cout << "\twrite [path]:\t" << "Replay write trace in MSR" << endl;
        cout << "\tread [path]:\t" << "Replay read trace in MSR" << endl;
        cout << "\trepair [path]:\t" << "Replay repair trace in MSR" << endl;
        cout << "\thybrid [path]:\t" << "Replay write and read trace in MSR" << endl;
    } else {
        manage = new Manage((config_path + "/" + string(argv[1])).c_str());
        if(string(argv[2]) == "-c") {
            create();
        } else if(string(argv[2]) == "-s") {
            if(string(argv[3]) == "write") {
                update_sample();
            } else if (string(argv[3]) == "read") {
                read_sample();
            } else if (string(argv[3]) == "repair") {
                degard_read_sample();
            }
        } else if(string(argv[2]) == "-i") {
            int rounds = atoi(argv[4]);
            int update_num = atoi(argv[5]);
            int64_t update_size = atoll(argv[6]);

            if(string(argv[3]) == "write") {
                update_iops(rounds, update_num, update_size);
            } else if (string(argv[3]) == "read") {
                read_iops(rounds, update_num, update_size);
            } else if (string(argv[3]) == "repair") {
                degard_read_iops(rounds, update_num, update_size);
            }
        } else if(string(argv[2]) == "-t") {
            if(string(argv[3]) == "write") {
                update_traces(argv[4]);
            } else if (string(argv[3]) == "read") {
                read_traces(argv[4]);
            } else if (string(argv[3]) == "repair") {
                degard_read_traces(argv[4]);
            } else if (string(argv[3]) == "hybrid") {
                hybrid_traces(argv[4]);
            }
        } else if(string(argv[2]) == "-d") {
            manage->clear();
        }
    }
    
    delete manage;
    return 0;
}

void create() {
    int chunkNum = manage->setting->data;
    unsigned long length = chunkNum * manage->setting->size;
    char* data = new char[length];

    createNewData(data, length);

    // 创建数据
    manage->create(data, length);

    delete [] data;
}

void update_sample() {
    // 加载trace文件
    vector<Trace> traces;
    int64_t segSize = manage->setting->data * manage->setting->size;
    ifstream ifs(sample_path, ios::in);

    if(ifs.is_open()) {
        while(!ifs.eof()) {
            char temp[100];
            ifs.getline(temp, 100);
            vector<string> str;
            boost::split(str, temp, boost::is_any_of(","), boost::token_compress_on);
            if(str[3] == "Write") {
                Trace trace(atoll(str[4].data()), atoll(str[5].data()));

                // trace正规化
                if(trace.length < manage->setting->part) {
                    trace.length = manage->setting->part;
                }
                if(trace.length % manage->setting->part != 0) {
                    trace.length = ceil((double)trace.length / manage->setting->part) * manage->setting->part;
                }

                trace.offset = ((trace.offset % segSize) / manage->setting->part) * manage->setting->part;

                int cid = trace.offset / manage->setting->size;
                int64_t end = (cid + 1) * manage->setting->size - trace.offset;
                if(trace.length > end) {
                    trace.offset -= trace.length;
                }
                traces.push_back(trace);
            }
        }

        for (int i = 0; i < traces.size(); ++i) {
            char* data = new char[traces[i].length];
            cout << "=============进行第" << i + 1 << "次更新==============" << endl;
            string file_name = update_sample_path + "/" + to_string(i + 1) + ".updataData.bin";
            createUpdateData(data, traces[i].length, file_name);
            manage->write(data, traces[i].offset, traces[i].length);
            cout << "=======================================" << endl;
            delete [] data;
        }
    }
}

void read_sample() {
    // 加载trace文件
    vector<Trace> traces;
    int64_t segSize = manage->setting->data * manage->setting->size;
    ifstream ifs(sample_path, ios::in);

    if(ifs.is_open()) {
        while(!ifs.eof()) {
            char temp[100];
            ifs.getline(temp, 100);
            vector<string> str;
            boost::split(str, temp, boost::is_any_of(","), boost::token_compress_on);
            if(str[3] == "Read") {
                Trace trace(atoll(str[4].data()), atoll(str[5].data()));

                // trace正规化
                if(trace.length < manage->setting->part) {
                    trace.length = manage->setting->part;
                }
                if(trace.length % manage->setting->part != 0) {
                    trace.length = ceil((double)trace.length / manage->setting->part) * manage->setting->part;
                }

                trace.offset = ((trace.offset % segSize) / manage->setting->part) * manage->setting->part;

                int cid = trace.offset / manage->setting->size;
                int64_t end = (cid + 1) * manage->setting->size - trace.offset;
                if(trace.length > end) {
                    trace.length = end;
                }

                traces.push_back(trace);
            }
        }

        for (int i = 0; i < traces.size(); ++i) {
            char* data = new char[traces[i].length];
            cout << "=============进行第" << i + 1 << "次读取==============" << endl;
            manage->read(data, traces[i].offset, traces[i].length);
            cout << "=======================================" << endl;
            delete [] data;
        }
    }
}

void degard_read_sample() {
    // 加载trace文件
    vector<Trace> traces;
    int64_t segSize = manage->setting->data * manage->setting->size;
    ifstream ifs(sample_path, ios::in);

    if(ifs.is_open()) {
        while(!ifs.eof()) {
            char temp[100];
            ifs.getline(temp, 100);
            vector<string> str;
            boost::split(str, temp, boost::is_any_of(","), boost::token_compress_on);
            if(str[3] == "Read") {
                Trace trace(atoll(str[4].data()), atoll(str[5].data()));

                // trace正规化
                if(trace.length < manage->setting->part) {
                    trace.length = manage->setting->part;
                }
                if(trace.length % manage->setting->part != 0) {
                    trace.length = ceil((double)trace.length / manage->setting->part) * manage->setting->part;
                }

                trace.offset = ((trace.offset % segSize) / manage->setting->part) * manage->setting->part;

                int cid = trace.offset / manage->setting->size;
                int64_t end = (cid + 1) * manage->setting->size - trace.offset;
                if(trace.length > end) {
                    trace.length = end;
                }

                traces.push_back(trace);
            }
        }

        for (int i = 0; i < traces.size(); ++i) {
            char* data = new char[traces[i].length];
            cout << "=============进行第" << i + 1 << "次恢复==============" << endl;
            manage->repair(data, traces[i].offset, traces[i].length, comm::DataType::DR);
            cout << "=======================================" << endl;
            delete [] data;
        }
    }
}

void update_iops(int rounds, int num, int64_t update_size) {
    int64_t offsets[num];
    int64_t step = (manage->setting->data * manage->setting->size) / num;
    for (int i = 0; i < num; ++i) {
        offsets[i] = i * step;
    }

    int index = 1;
    char* data = new char[update_size];
    for (int i = 0; i < rounds; ++i) {
        cout << "==========第" << (i + 1) << "轮更新==========" << endl;
        for (int j = 0; j < num; ++j) {
            cout << "=============进行第" << j + 1 << "次更新==============" << endl;
            string file_name = update_iops_path + "/" + to_string(index) + ".updataData.bin";
            createUpdateData(data, update_size, file_name);
            manage->write(data, offsets[j], update_size);
            cout << "=======================================" << endl;
            index++;
        }
        cout << "==================================\n" << endl;
        cout << endl;
    }
    delete [] data;
}

void read_iops(int rounds, int num, int64_t update_size) {
    int64_t offsets[num];
    int64_t step = (manage->setting->data * manage->setting->size) / num;
    for (int i = 0; i < num; ++i) {
        offsets[i] = i * step;
    }

    int index = (rounds - 1) * num + 1;
    char* data = new char[update_size];
    char* temp = new char[update_size];
    for (int i = 0; i < num; ++i) {
        cout << "=============进行第" << i + 1 << "次读取==============" << endl;
        string file_name = update_iops_path + "/" + to_string(index) + ".updataData.bin";
        createUpdateData(temp, update_size, file_name);
        manage->read(data, offsets[i], update_size);

        bool flag = true;
        for (int j = 0; j < update_size; ++j) {
            if(temp[j] != data[j]) {
                cout << j << ": 读取失败！" << endl;
                flag = false;
                break;
            }
        }
        if(flag) {
            cout << "读取成功！" << endl;
        }
        cout << "=======================================" << endl;
        cout << endl;

        index++;
    }
    delete [] data;
    delete [] temp;
}
void degard_read_iops(int rounds, int num, int64_t update_size) {
    int64_t offsets[num];
    int64_t step = (manage->setting->data * manage->setting->size) / num;
    for (int i = 0; i < num; ++i) {
        offsets[i] = i * step;
    }

    int index = (rounds - 1) * num + 1;
    char* data = new char[update_size];
    char* temp = new char[update_size];
    for (int i = 0; i < num; ++i) {
        cout << "=============进行第" << i + 1 << "次恢复==============" << endl;
        string file_name = update_iops_path + "/" + to_string(index) + ".updataData.bin";
        createUpdateData(temp, update_size, file_name);
        manage->repair(data, offsets[i], update_size, comm::DataType::DR);

        bool flag = true;
        for (int j = 0; j < update_size; ++j) {
            if(temp[j] != data[j]) {
                cout << j << ": 恢复失败！" << endl;
                flag = false;
                break;
            }
        }
        if(flag) {
            cout << "恢复成功！" << endl;
        }
        cout << "=======================================" << endl;
        cout << endl;
        index++;
    }
    delete [] data;
    delete [] temp;
}

void update_traces(string filename) {
    vector<Trace> traces;
    ifstream ifs(traces_path + "/write/" + filename, ios::in);

    if(ifs.is_open()) {
        while(!ifs.eof()) {
            char temp[100];
            ifs.getline(temp, 100);
            if(temp[0] == '\0') break;
            vector<string> str;
            boost::split(str, temp, boost::is_any_of(","), boost::token_compress_on);
            traces.push_back(Trace(atoll(str[3].data()), atoll(str[4].data())));
        }

        for (int i = 0; i < traces.size(); ++i) {
            char* data = new char[traces[i].length];
            cout << "=============进行第" << i + 1 << "次更新==============" << endl;
            string file_name = update_trace_path + "/" + filename + ".dir/" + to_string(i + 1) + ".updataData.bin";
            createUpdateData(data, traces[i].length, file_name);
            manage->write(data, traces[i].offset, traces[i].length);
            cout << "=======================================" << endl;
            delete [] data;
        }
    }
}

void read_traces(string filename) {
    vector<Trace> traces;
    ifstream ifs(traces_path + "/read/" + filename, ios::in);

    if(ifs.is_open()) {
        while(!ifs.eof()) {
            char temp[100];
            ifs.getline(temp, 100);
            if(temp[0] == '\0') break;
            vector<string> str;
            boost::split(str, temp, boost::is_any_of(","), boost::token_compress_on);
            traces.push_back(Trace(atoll(str[3].data()), atoll(str[4].data())));
        }

        for (int i = 0; i < traces.size(); ++i) {
            char* data = new char[traces[i].length];
            cout << "=============进行第" << i + 1 << "次读取==============" << endl;
            manage->read(data, traces[i].offset, traces[i].length);
            cout << "=======================================" << endl;
            delete [] data;
        }
    }
}

void hybrid_traces(string filename) {
    vector<Trace> traces;
    ifstream ifs(traces_path + "/hybrid/" + filename, ios::in);

    if(ifs.is_open()) {
        while(!ifs.eof()) {
            char temp[100];
            ifs.getline(temp, 100);
            if(temp[0] == '\0') break;
            vector<string> str;
            boost::split(str, temp, boost::is_any_of(","), boost::token_compress_on);
            traces.push_back(Trace(str[0],atoll(str[3].data()), atoll(str[4].data())));
        }

        int index = 1;
        for (int i = 0; i < traces.size(); ++i) {
            char* data = new char[traces[i].length];
            cout << "=============进行第" << i + 1 << "次IO==============" << endl;
            if(traces[i].type == "Write") {
                string file_name = update_hybrid_path + "/" + filename + ".dir/" + to_string(index) + ".updataData.bin";
                createUpdateData(data, traces[i].length, file_name);
                manage->write(data, traces[i].offset, traces[i].length);
                index++;
            } else if(traces[i].type == "Read") {
                manage->read(data, traces[i].offset, traces[i].length);
            }
            cout << "=======================================" << endl;
            delete [] data;
        }
    }
}

void degard_read_traces(string filename) {
    vector<Trace> traces;
    ifstream ifs(traces_path + "/repair/" + filename, ios::in);

    if(ifs.is_open()) {
        while(!ifs.eof()) {
            char temp[100];
            ifs.getline(temp, 100);
            if(temp[0] == '\0') break;
            vector<string> str;
            boost::split(str, temp, boost::is_any_of(","), boost::token_compress_on);
            traces.push_back(Trace(str[0], atoll(str[3].c_str()), atoll(str[4].c_str())));
        }

        // 更新
        int index = 1;
        for (int i = 0; i < traces.size(); ++i) {
            if(traces[i].type == "Write") {
                char* data = new char[traces[i].length];
                cout << "=============进行第" << index << "次更新==============" << endl;
                string file_name = update_repair_path + "/" + filename + ".dir/" + to_string(index) + ".updataData.bin";
                createUpdateData(data, traces[i].length, file_name);
                manage->write(data, traces[i].offset, traces[i].length);
                cout << "=======================================" << endl;
                delete [] data;
                index++;
            }
        }

        cout << endl;
        cout << endl;
        cout << endl;
        cout << "更新完成，开始修复......" << endl;
        cout << endl;
        cout << endl;
        cout << endl;

        // 修复
        index = 1;
        for (int i = 0; i < traces.size(); ++i) {
            if(traces[i].type == "Read") {
                char* data = new char[traces[i].length];
                cout << "=============进行第" << index << "次恢复==============" << endl;
                manage->repair(data, traces[i].offset, traces[i].length, comm::DataType::DR);
                cout << "=======================================" << endl;
                delete [] data;
                index++;
            }
        }
    }
}

void createNewData(char* data, int64_t length) {
    ifstream ifs(config_path + "/user_data.bin", ios::in | ios::binary);
    if(ifs.is_open()) {
        ifs.read(data, length);
        ifs.close();
    } else {
        // 用户数据不存在，则创建新数据！
        srand((unsigned )time(NULL));
        for(int i = 0; i < length; i ++) {
            data[i] = rand()%(127 - (-128) + 1) + (-128);
        }
        ofstream ofs(config_path + "/user_data.bin", ios::out | ios::binary);
        ofs.write(data, length);
        ofs.close();
    }
}

void createUpdateData(char* data, int64_t length, string path) {
    ifstream ifs(path, ios::in | ios::binary);
    if(ifs.is_open()) {
        ifs.read(data, length);
        ifs.close();
    } else {
        // 用户数据不存在，则创建新数据！
        srand((unsigned )time(NULL));
        for(int i = 0; i < length; i ++) {
            data[i] = rand()%(127 - (-128) + 1) + (-128);
        }
        ofstream ofs(path, ios::out | ios::binary);
        ofs.write(data, length);
        ofs.close();
    }
}