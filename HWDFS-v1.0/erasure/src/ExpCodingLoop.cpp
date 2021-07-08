#include "ExpCodingLoop.h"

void ByteInputOutputExpCodingLoop::codeSomeShards(
        CVec2D &matrixRows,
        CVec2D &inputs,
        int inputCount,
        CVec2D &outputs,
        int outputCount,
        int offset,
        int byteCount) {

    for (int iByte = offset; iByte < offset + byteCount; iByte++) {
        {
            int iInput = 0;
            CVec &inputShard = inputs[iInput];
            char inputByte = inputShard[iByte];
            for (int iOutput = 0; iOutput < outputCount; iOutput++) {
                CVec &outputShard = outputs[iOutput];
                CVec &matrixRow = matrixRows[iOutput];
                outputShard[iByte] = Galois::multiply(matrixRow[iInput], inputByte);
            }
        }

        for (int iInput = 1; iInput < inputCount; iInput++) {
            CVec &inputShard = inputs[iInput];
            char inputByte = inputShard[iByte];
            for (int iOutput = 0; iOutput < outputCount; iOutput++) {
                CVec &outputShard = outputs[iOutput];
                CVec &matrixRow = matrixRows[iOutput];
                outputShard[iByte] ^= Galois::multiply(matrixRow[iInput], inputByte);
            }
        }
    }
}

void ByteOutputInputExpCodingLoop::codeSomeShards(
        CVec2D &matrixRows,
        CVec2D &inputs,
        int inputCount,
        CVec2D &outputs,
        int outputCount,
        int offset,
        int byteCount) {

    for (int iByte = offset; iByte < offset + byteCount; iByte++) {
        for (int iOutput = 0; iOutput < outputCount; iOutput++) {
            CVec &matrixRow = matrixRows[iOutput];
            int value = 0;
            for (int iInput = 0; iInput < inputCount; iInput++) {
                value ^= Galois::multiply(matrixRow[iInput], inputs[iInput][iByte]);
            }
            outputs[iOutput][iByte] = (char) value;
        }
    }
}

void InputByteOutputExpCodingLoop::codeSomeShards(
        CVec2D &matrixRows,
        CVec2D &inputs,
        int inputCount,
        CVec2D &outputs,
        int outputCount,
        int offset,
        int byteCount) {

    {
        int iInput = 0;
        CVec &inputShard = inputs[iInput];
        for (int iByte = offset; iByte < offset + byteCount; iByte++) {
            char inputByte = inputShard[iByte];
            for (int iOutput = 0; iOutput < outputCount; iOutput++) {
                CVec &outputShard = outputs[iOutput];
                CVec &matrixRow = matrixRows[iOutput];
                outputShard[iByte] = Galois::multiply(matrixRow[iInput], inputByte);
            }
        }
    }

    for (int iInput = 1; iInput < inputCount; iInput++) {
        CVec &inputShard = inputs[iInput];
        for (int iByte = offset; iByte < offset + byteCount; iByte++) {
            char inputByte = inputShard[iByte];
            for (int iOutput = 0; iOutput < outputCount; iOutput++) {
                CVec &outputShard = outputs[iOutput];
                CVec &matrixRow = matrixRows[iOutput];
                outputShard[iByte] ^= Galois::multiply(matrixRow[iInput], inputByte);
            }
        }
    }
}

void InputOutputByteExpCodingLoop::codeSomeShards(
        CVec2D &matrixRows,
        CVec2D &inputs,
        int inputCount,
        CVec2D &outputs,
        int outputCount,
        int offset,
        int byteCount) {
    {
        int iInput = 0;
        CVec &inputShard = inputs[iInput];
        for (int iOutput = 0; iOutput < outputCount; iOutput++) {
            CVec &outputShard = outputs[iOutput];
            CVec &matrixRow = matrixRows[iOutput];
            char matrixByte = matrixRow[iInput];
            for (int iByte = offset; iByte < offset + byteCount; iByte++) {
                outputShard[iByte] = Galois::multiply(matrixByte, inputShard[iByte]);
            }
        }
    }

    for (int iInput = 1; iInput < inputCount; iInput++) {
        CVec &inputShard = inputs[iInput];
        for (int iOutput = 0; iOutput < outputCount; iOutput++) {
            CVec &outputShard = outputs[iOutput];
            CVec &matrixRow = matrixRows[iOutput];
            char matrixByte = matrixRow[iInput];
            for (int iByte = offset; iByte < offset + byteCount; iByte++) {
                outputShard[iByte] ^= Galois::multiply(matrixByte, inputShard[iByte]);
            }
        }
    }
}

void OutputByteInputExpCodingLoop::codeSomeShards(
        CVec2D &matrixRows,
        CVec2D &inputs,
        int inputCount,
        CVec2D &outputs,
        int outputCount,
        int offset,
        int byteCount) {

    for (int iOutput = 0; iOutput < outputCount; iOutput++) {
        CVec &outputShard = outputs[iOutput];
        CVec &matrixRow = matrixRows[iOutput];
        for (int iByte = offset; iByte < offset + byteCount; iByte++) {
            int value = 0;
            for (int iInput = 0; iInput < inputCount; iInput++) {
                CVec &inputShard = inputs[iInput];
                value ^= Galois::multiply(matrixRow[iInput], inputShard[iByte]);
            }
            outputShard[iByte] = (char)value;
        }
    }
}

void OutputInputByteExpCodingLoop::codeSomeShards(
        CVec2D &matrixRows,
        CVec2D &inputs,
        int inputCount,
        CVec2D &outputs,
        int outputCount,
        int offset,
        int byteCount) {
    for (int iOutput = 0; iOutput < outputCount; iOutput++) {
        CVec &outputShard = outputs[iOutput];
        CVec &matrixRow = matrixRows[iOutput];
        {
            int iInput = 0;
            CVec &inputShard = inputs[iInput];
            char matrixByte = matrixRow[iInput];
            for (int iByte = offset; iByte < offset + byteCount; iByte++) {
                outputShard[iByte] = Galois::multiply(matrixByte, inputShard[iByte]);
            }
        }
        for (int iInput = 1; iInput < inputCount; iInput++) {
            CVec &inputShard = inputs[iInput];
            char matrixByte = matrixRow[iInput];
            for (int iByte = offset; iByte < offset + byteCount; iByte++) {
                outputShard[iByte] ^= Galois::multiply(matrixByte, inputShard[iByte]);
            }
        }
    }
}