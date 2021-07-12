#pragma once

#include "Shared.h"
#include "Galois.h"
#include "Matrix.h"
#include "CodingLoop.h"

class ReedSolomon
{
private:
    CodingLoop* codingLoop = NULL;

public:
    int dataShardCount;
    int parityShardCount;
    int totalShardCount;
    Matrix matrix;
    CVec2D parityRows;

    static ReedSolomon* create(int dataShardCount, int parityShardCount);
    ReedSolomon(int dataShardCount, int parityShardCount, CodingLoop* codingLoop);
    ~ReedSolomon();
    void encodeParity(CVec2D &shards, int offset, int byteCount);
    bool isParityCorrect(CVec2D &shards, int firstByte, int byteCount);
    void decodeMissing(CVec2D &shards, BVec &shardPresent, int offset, int byteCount);
    void checkBuffersAndSizes(CVec2D &shards, int offset, int byteCount);
    static Matrix buildMatrix(int dataShards, int totalShards);
    static Matrix vandermonde(int rows, int cols);
};