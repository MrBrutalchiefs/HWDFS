#pragma once

#include "Shared.h"
#include "Galois.h"

class CodingLoop {
public:
    virtual void codeSomeShards(
            CVec2D &matrixRows,
            CVec2D &inputs,
            int inputCount,
            CVec2D &outputs,
            int outputCount,
            int offset,
            int byteCount) = 0;
    virtual bool checkSomeShards(
            CVec2D &matrixRows,
            CVec2D &inputs,
            int inputCount,
            CVec2D &toCheck,
            int checkCount,
            int offset,
            int byteCount);
};
