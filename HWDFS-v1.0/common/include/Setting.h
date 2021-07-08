#pragma once

#include <string>
#include "StoreService.h"
#include "ServerInfo.h"
#include "ReedSolomon.h"

using namespace std;
using namespace comm;

class Setting {
public:
    int data;
    int parity;
    int total;
    unsigned long size;
    unsigned long part;
    string scheme;

    int part_num;
    ReedSolomon* rs;
    vector<ServerInfo*> servers;
    vector<StoreServiceClient*> clients;

    Setting() {}
    Setting(int data, int parity, unsigned long size, unsigned long part, string scheme);

    ~Setting();

};

