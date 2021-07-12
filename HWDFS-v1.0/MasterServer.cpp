#include "MasterService.h"
#include <cppconn/driver.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/ThreadFactory.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <Setting.h>
#include <RSConfiguration.h>
#include <Buffer.h>

using namespace std;
using namespace comm;
using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;

Setting* setting = RSConfiguration::load((char *)"config.xml");

class MasterServiceHandler : virtual public MasterServiceIf {
public:
    MasterServiceHandler() {}

    void getChunkServer(MetaResponse& _return, const MetaRequest& request) {
        if (setting->scheme == "REPLICATION" || setting->scheme == "REPLICATION_LOG") {
            ChunkInfo info;
            info.user_id = request.user_id;
            info.offset = request.offset;
            info.length = request.length;
            _return.info_list.push_back(info);
        } else {
            vector<vector<int64_t>> segments = splitToSegment(request.offset, request.length, setting);
            for (vector<int64_t> segment : segments) {
                vector<ChunkInfo> chunks = splitToChunk(segment[1], segment[2], setting);
                for (ChunkInfo info : chunks) {
                    info.user_id = request.user_id;
                    info.segment_id = segment[0];

                    _return.info_list.push_back(info);
                }
            }
        }
        _return.code = ResponseCode::SUCCESS;
    }

private:
    vector<vector<int64_t>> splitToSegment(int64_t offset, int64_t length, Setting* setting) {
        vector<vector<int64_t>> segments;
        int64_t segLength = setting->size * setting->data;

        vector<int64_t> begin_segment = getSegmentInfoByOffset(offset, segLength);
        vector<int64_t> end_segment = getSegmentInfoByOffset(offset + length - 1, segLength);

        if(begin_segment[0] == end_segment[0]) {
            // 若数据仅在一个条带内
            begin_segment[2] = length;
            segments.push_back(begin_segment);
        } else {
            // 若数据在多个条带内
            segments.push_back(begin_segment);
            for (int16_t i = begin_segment[0] + 1; i < end_segment[0]; ++i) {
                vector<int64_t> segment;
                segment[0] = i;
                segment[1] = 0;
                segment[2] = segLength;
            }
            end_segment[2] = end_segment[1] + 1;
            end_segment[1] = 0;
            segments.push_back(end_segment);
        }

        return segments;
    }

    vector<ChunkInfo> splitToChunk(int64_t offset, int64_t length, Setting* setting) {
        vector<ChunkInfo> info_list;

        ChunkInfo begin_chunk = getChunkInfoByOffset(offset, setting->size);
        ChunkInfo end_chunk = getChunkInfoByOffset(offset + length - 1, setting->size);

        if(begin_chunk.chunk_id == end_chunk.chunk_id) {
            // 若数据仅在一个块内
            begin_chunk.length = length;
            info_list.push_back(begin_chunk);
        } else {
            // 若涉及多个块
            info_list.push_back(begin_chunk);
            for (int i = begin_chunk.chunk_id + 1; i < end_chunk.chunk_id; ++i) {
                ChunkInfo info;
                info.chunk_id = i;
                info.offset = 0;
                info.length = setting->size;
                info_list.push_back(info);
            }
            end_chunk.length = end_chunk.offset + 1;
            end_chunk.offset = 0;
            info_list.push_back(end_chunk);
        }
        return info_list;
    }

    vector<int64_t> getSegmentInfoByOffset(int64_t offset, int64_t segLength) {
        vector<int64_t> segment(3);
        segment[0] = offset / segLength;
        segment[1] = offset % segLength;
        segment[2] = segLength - offset;
        return segment;
    }

    ChunkInfo getChunkInfoByOffset(int64_t offset, int64_t chunk_size) {
        ChunkInfo info;
        info.chunk_id = offset / chunk_size;
        info.offset = offset % chunk_size;
        info.length = chunk_size - offset;
        return info;
    }

};

class MasterServiceCloneFactory : virtual public MasterServiceIfFactory {
public:
    ~MasterServiceCloneFactory() override = default;
    MasterServiceIf* getHandler(const ::apache::thrift::TConnectionInfo& connInfo) override{
        std::shared_ptr<TSocket> sock = std::dynamic_pointer_cast<TSocket>(connInfo.transport);
        return new MasterServiceHandler;
    }

    void releaseHandler(::comm::MasterServiceIf* handler) override {
        delete handler;
    }
};

int main(int argc, char **argv) {
    int port = 9090;
    cout << "请输入Master服务器运行的端口号: ";
    cin >> port;

    // 服务器一次允许6个连接
    int workerCount = setting->total;
    std::shared_ptr<ThreadManager> threadManager =
            ThreadManager::newSimpleThreadManager(workerCount);
    threadManager->threadFactory(std::make_shared<ThreadFactory>());
    threadManager->start();

    TThreadPoolServer server(
            std::make_shared<MasterServiceProcessorFactory>(std::make_shared<MasterServiceCloneFactory>()),
            std::make_shared<TServerSocket>(port),
            std::make_shared<TBufferedTransportFactory>(),
            std::make_shared<TBinaryProtocolFactory>(),
            threadManager);
    server.serve();

    return 0;
}

