#pragma once

#include "CodingLoop.h"

class ByteInputOutputTableCodingLoop : public CodingLoop {
public:
    void codeSomeShards(CVec2D &matrixRows,
                        CVec2D &inputs,
                        int inputCount,
                        CVec2D &outputs,
                        int outputCount,
                        int offset,
                        int byteCount) override;
};

class ByteOutputInputTableCodingLoop : public CodingLoop {
public:
    void codeSomeShards(CVec2D &matrixRows,
                        CVec2D &inputs,
                        int inputCount,
                        CVec2D &outputs,
                        int outputCount,
                        int offset,
                        int byteCount) override;
};

class InputByteOutputTableCodingLoop : public CodingLoop {
public:
    void codeSomeShards(CVec2D &matrixRows,
                        CVec2D &inputs,
                        int inputCount,
                        CVec2D &outputs,
                        int outputCount,
                        int offset,
                        int byteCount) override;
};

class InputOutputByteTableCodingLoop : public CodingLoop {
public:
    void codeSomeShards(CVec2D &matrixRows,
                        CVec2D &inputs,
                        int inputCount,
                        CVec2D &outputs,
                        int outputCount,
                        int offset,
                        int byteCount) override;

//    bool checkSomeShards(
//            CVec2D &matrixRows,
//            CVec2D &inputs,
//            int inputCount,
//            CVec2D &toCheck,
//            int checkCount,
//            int offset,
//            int byteCount) override;
};

class OutputByteInputTableCodingLoop : public CodingLoop {
public:
    void codeSomeShards(CVec2D &matrixRows,
                        CVec2D &inputs,
                        int inputCount,
                        CVec2D &outputs,
                        int outputCount,
                        int offset,
                        int byteCount) override;
};

class OutputInputByteTableCodingLoop : public CodingLoop {
public:
    void codeSomeShards(CVec2D &matrixRows,
                        CVec2D &inputs,
                        int inputCount,
                        CVec2D &outputs,
                        int outputCount,
                        int offset,
                        int byteCount) override;

//    bool checkSomeShards(
//            CVec2D &matrixRows,
//            CVec2D &inputs,
//            int inputCount,
//            CVec2D &toCheck,
//            int checkCount,
//            int offset,
//            int byteCount) override;
};