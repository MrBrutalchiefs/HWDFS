#include "ServerInfo.h"

ServerInfo::ServerInfo() {

}

ServerInfo::ServerInfo(int id, string ip, int port, string path) {
    this->id = id;
    this->ip = ip;
    this->port = port;
    this->path = path;
}

/*
ServerInfo::ServerInfo(int id, string ip, int port, string path, string type) {
    this->id = id;
    this->ip = ip;
    this->port = port;
    this->path = path;
    this->type = type;
}
*/

/*
ServerInfo::ServerInfo(int id, string ip, int port, string path, string type,
                       string DBHOST, string USER, string PASSWORD) {
    this->id = id;
    this->ip = ip;
    this->port = port;
    this->path = path;
    this->type = type;

    this->DBHOST = DBHOST;
    this->USER = USER;
    this->PASSWORD = PASSWORD;
}
*/
