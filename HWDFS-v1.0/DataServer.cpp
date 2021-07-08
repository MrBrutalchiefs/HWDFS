#include <iostream>
#include <cppconn/driver.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/ThreadFactory.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include "StoreService.h"
#include "Manage.h"
#include "Scheme.h"
#include "SchemeFactory.h"

using namespace std;
using namespace comm;
using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;

Setting* setting = RSConfiguration::load((char *)"config.xml");
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
        scheme->writeData(_return, request, buffer, setting);
        delete [] scheme;
    }

    void read(Response& _return, const Request& request) {
        Scheme* scheme = SchemeFactory::createScheme(request.scheme);
        scheme->readData(_return, request, buffer, setting);
        delete [] scheme;
    }

    void degrade_read(Response& _return, const Request& request) {
        Scheme* scheme = SchemeFactory::createScheme(request.scheme);
        scheme->degrade_read(_return, request, buffer, setting);
        delete [] scheme;
    }

    void merge(Response& _return, const Request& request) {
        Scheme* scheme = SchemeFactory::createScheme(request.scheme);
        scheme->mergeData(_return, request, buffer, setting);
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

int main() {
    int port = 9090;

    // 服务器一次允许6个连接
    int workerCount = setting->total;
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

    /*
    ::std::shared_ptr<StoreServiceHandler> handler(new StoreServiceHandler());
    ::std::shared_ptr<TProcessor> processor(new StoreServiceProcessor(handler));
    ::std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    ::std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    ::std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
    */

    server.serve();

    return 0;
}

