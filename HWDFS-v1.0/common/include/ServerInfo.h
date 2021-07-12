#pragma once

#include <string>

using namespace std;

class ServerInfo {
public:
    int id;
    string ip;
    int port;
    string path;
//    string type;
//    string DBHOST;
//    string USER;
//    string PASSWORD;

    ServerInfo();
    ServerInfo(int id, string ip, int port, string path);
//    ServerInfo(int id, string ip, int port, string path, string type);
//    ServerInfo(int id, string ip, int port, string path, string type, string DBHOST, string USER, string PASSWORD);
};
