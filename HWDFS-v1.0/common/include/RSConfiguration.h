#pragma once
#include <iostream>
#include <string>
#include <cstdlib>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/protocol/TCompactProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include "ServerInfo.h"
#include "StoreService.h"

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace std;
using namespace comm;

#include "Setting.h"
#include "tinyxml.h"
#include "ServerInfo.h"

class RSConfiguration {
public:
    static Setting* load(const char * fileName);
};
