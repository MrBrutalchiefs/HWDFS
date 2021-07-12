#pragma once

#include "CodingLoop.h"

class ByteInputOutputExpCodingLoop : public CodingLoop {
public:
    void codeSomeShards(CVec2D &matrixRows,
                        CVec2D &inputs,
                        int inputCount,
                        CVec2D &outputs,
                        int outputCount,
                        int offset,
                        int byteCount) override;
};

class ByteOutputInputExpCodingLoop : public CodingLoop {
public:
    void codeSomeShards(CVec2D &matrixRows,
                        CVec2D &inputs,
                        int inputCount,
                        CVec2D &outputs,
                        int outputCount,
                        int offset,
                        int byteCount) override;
};

class InputByteOutputExpCodingLoop : public CodingLoop {
public:
    void codeSomeShards(CVec2D &matrixRows,
                        CVec2D &inputs,
                        int inputCount,
                        CVec2D &outputs,
                        int outputCount,
                        int offset,
                        int byteCount) override;
};

class InputOutputByteExpCodingLoop : public CodingLoop {
public:
    void codeSomeShards(CVec2D &matrixRows,
                        CVec2D &inputs,
                        int inputCount,
                        CVec2D &outputs,
                        int outputCount,
                        int offset,
                        int byteCount) override;
};

class OutputByteInputExpCodingLoop : public CodingLoop {
public:
    void codeSomeShards(CVec2D &matrixRows,
                        CVec2D &inputs,
                        int inputCount,
                        CVec2D &outputs,
                        int outputCount,
                        int offset,
                        int byteCount) override;
};

class OutputInputByteExpCodingLoop : public CodingLoop {
public:
    void codeSomeShards(CVec2D &matrixRows,
                        CVec2D &inputs,
                        int inputCount,
                        CVec2D &outputs,
                        int outputCount,
                        int offset,
                        int byteCount) override;
};