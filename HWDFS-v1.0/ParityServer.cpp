#include <iostream>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/ThreadFactory.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "Manage.h"
#include "StoreService.h"
#include "Scheme.h"
#include "SchemeFactory.h"

using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;
using namespace std;
using namespace comm;

Setting* setting = RSConfiguration::load((char*)"config.xml");
Buffer* buffer = new Buffer(*setting);

class StoreServiceHandler : virtual public StoreServiceIf {
public:
    StoreServiceHandler() {}

    ~StoreServiceHandler() {}

    void create(Response& _return, const Request& request) {
        Scheme* scheme = SchemeFactory::createScheme(request.scheme);
        scheme->create(_return, request, buffer, setting);
        delete [] scheme;
    }

    void write(Response& _return, const Request& request) {
        Scheme* scheme = SchemeFactory::createScheme(request.scheme);
        scheme->writeParity(_return, request, buffer, setting);
        delete [] scheme;
    }

    void read(Response& _return, const Request& request) {
        Scheme* scheme = SchemeFactory::createScheme(request.scheme);
        scheme->readParity(_return, request, buffer, setting);
        delete [] scheme;
    }

    void degrade_read(Response& _return, const Request& request) {}

    void merge(Response& _return, const Request& request) {
        Scheme* scheme = SchemeFactory::createScheme(request.scheme);
        scheme->mergeParity(_return, request, buffer, setting);
        delete [] scheme;
    }

    void clear(Response& _return, const Request& request) {
        Scheme* scheme = SchemeFactory::createScheme(request.scheme);
        scheme->clear(_return, request, buffer, setting);
        delete [] scheme;
    }
};

class StoreServiceCloneFactory : virtual public StoreServiceIfFactory {
public:
    ~StoreServiceCloneFactory() override = default;
    StoreServiceIf* getHandler(const ::apache::thrift::TConnectionInfo& connInfo) override{
        std::shared_ptr<TSocket> sock = std::dynamic_pointer_cast<TSocket>(connInfo.transport);
        return new StoreServiceHandler;
    }

    void releaseHandler(::comm::StoreServiceIf* handler) override {
        delete handler;
    }
};

int main(int argc, char **argv) {
    int port = 9090;
    cout << "请输入Parity服务器运行的端口号: ";
    cin >> port;

    /*
    std::shared_ptr<StoreServiceHandler> handler(new StoreServiceHandler());
    std::shared_ptr<TProcessor> processor(new StoreServiceProcessor(handler));
    std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
    TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
    */

    // 服务器一次允许16个连接
    int workerCount = 16;
    std::shared_ptr<ThreadManager> threadManager =
            ThreadManager::newSimpleThreadManager(workerCount);
    threadManager->threadFactory(std::make_shared<ThreadFactory>());
    threadManager->start();

    TThreadPoolServer server(
            std::make_shared<StoreServiceProcessorFactory>(std::make_shared<StoreServiceCloneFactory>()),
            std::make_shared<TServerSocket>(port),
            std::make_shared<TBufferedTransportFactory>(),
            std::make_shared<TBinaryProtocolFactory>(),
            threadManager);

    server.serve();
    return 0;
}

