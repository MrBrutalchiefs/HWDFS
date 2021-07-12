#include "Setting.h"

Setting::Setting(int data, int parity, unsigned long size, unsigned long part, string scheme) {
    this->data = data;
    this->parity = parity;
    this->total = data + parity;
    this->size = size;
    this->part = part;
    this->scheme = scheme;

    this->part_num = size / part;
    this->rs = ReedSolomon::create(data, parity);
    this->servers = vector<ServerInfo*>(this->total);
    this->clients = vector<StoreServiceClient*>(this->total);
}

Setting::~Setting() {
    if(this->rs != NULL) {
        delete this->rs;
        this->rs = NULL;
    }
    for (int i = 0; i < this->total; ++i) {
        if(this->servers[i] != NULL) {
            delete this->servers[i];
            this->servers[i] = NULL;
        }
        if(this->clients[i] != NULL) {
            delete clients[i];
            this->clients[i] = NULL;
        }
    }
}
