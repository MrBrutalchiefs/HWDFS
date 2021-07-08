#include "RSConfiguration.h"

Setting* RSConfiguration::load(const char * fileName) {
    TiXmlDocument doc;
    if(!doc.LoadFile(fileName)){
        cerr << doc.ErrorDesc() << endl;
        exit(1);
    }

    TiXmlElement* root = doc.FirstChildElement();
    if(root == NULL) {
        cerr << "Failed to load file: No root element." << endl;
        doc.Clear();
        exit(1);
    }

    // 读取数据块的个数
    TiXmlElement* elem = root->FirstChildElement();
    int dataChunk = atoi(elem->GetText());

    // 读取校验块的个数
    elem = elem->NextSiblingElement();
    int parityChunk = atoi(elem->GetText());

    int total = dataChunk + parityChunk;

    // 读取块的大小
    elem = elem->NextSiblingElement();
    string temp = elem->GetText();
    unsigned long chunkSize = atol(temp.substr(0, temp.length() - 1).c_str());
    if(temp[temp.length() - 1] == 'K') {
        chunkSize *= 1024UL;
    } else if(temp[temp.length() - 1] == 'M') {
        chunkSize *= (1024UL * 1024UL);
    } else if(temp[temp.length() - 1] == 'G') {
        chunkSize *= (1024UL * 1024UL * 1024UL);
    }


    // 读取细分大小
    elem = elem->NextSiblingElement();
    temp = elem->GetText();
    unsigned long partSize = atol(temp.substr(0, temp.length() - 1).c_str());
    if(temp[temp.length() - 1] == 'K') {
        partSize *= 1024UL;
    } else if(temp[temp.length() - 1] == 'M') {
        partSize *= (1024UL * 1024UL);
    } else if(temp[temp.length() - 1] == 'G') {
        partSize *= (1024UL * 1024UL * 1024UL);
    }

    // 获取更新策略
    elem = elem->NextSiblingElement();
    TiXmlElement* sub_elem = elem->FirstChildElement()->FirstChildElement();
    string updateScheme = sub_elem->GetText();

    Setting* setting = new Setting(dataChunk, parityChunk, chunkSize, partSize, updateScheme);

    // 获取服务器信息并，初始化客户端信息
    elem = elem->NextSiblingElement();
    sub_elem = elem->FirstChildElement();
    for(int i = 0; i < total; i++) {
        // 初始化服务器信息
        TiXmlElement* server_elem = sub_elem->FirstChildElement();
        string ip = server_elem->GetText();

        server_elem = server_elem->NextSiblingElement();
        int port = atoi(server_elem->GetText());

        server_elem = server_elem->NextSiblingElement();
        string path = server_elem->GetText();

        setting->servers[i] = new ServerInfo(i, ip, port, path);
        sub_elem = sub_elem->NextSiblingElement();

        // 初始化客户端
        std::shared_ptr<TTransport> socket(new TSocket(ip, port));
        std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        setting->clients[i] = new StoreServiceClient(protocol);
    }
    doc.Clear();

    return setting;
}