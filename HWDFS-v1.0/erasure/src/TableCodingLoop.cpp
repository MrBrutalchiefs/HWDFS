#include "TableCodingLoop.h"
#include "Galois.h"

void ByteInputOutputTableCodingLoop::codeSomeShards(
        CVec2D &matrixRows,
        CVec2D &inputs,
        int inputCount,
        CVec2D &outputs,
        int outputCount,
        int offset, int byteCount) {

    CVec2D table = Galois::MULTIPLICATION_TABLE;
    for (int iByte = offset; iByte < offset + byteCount; iByte++) {
        {
            int iInput = 0;
            CVec &inputShard = inputs[iInput];
            char inputByte = inputShard[iByte];
            for (int iOutput = 0; iOutput < outputCount; iOutput++) {
                CVec &outputShard = outputs[iOutput];
                CVec &matrixRow = matrixRows[iOutput];
                CVec &multTableRow = table[matrixRow[iInput] & 0xFF];
                outputShard[iByte] = multTableRow[inputByte & 0xFF];
            }
        }
        for (int iInput = 1; iInput < inputCount; iInput++) {
            CVec &inputShard = inputs[iInput];
            char inputByte = inputShard[iByte];
            for (int iOutput = 0; iOutput < outputCount; iOutput++) {
                CVec &outputShard = outputs[iOutput];
                CVec &matrixRow = matrixRows[iOutput];
                CVec &multTableRow = table[matrixRow[iInput] & 0xFF];
                outputShard[iByte] ^= multTableRow[inputByte & 0xFF];
            }
        }
    }
}

void ByteOutputInputTableCodingLoop::codeSomeShards(
        CVec2D &matrixRows,
        CVec2D &inputs,
        int inputCount,
        CVec2D &outputs,
        int outputCount,
        int offset, int byteCount) {

    CVec2D table = Galois::MULTIPLICATION_TABLE;
    for (int iByte = offset; iByte < offset + byteCount; iByte++) {
        for (int iOutput = 0; iOutput < outputCount; iOutput++) {
            CVec &matrixRow = matrixRows[iOutput];
            int value = 0;
            for (int iInput = 0; iInput < inputCount; iInput++) {
                value ^= table[matrixRow[iInput] & 0xFF][inputs[iInput][iByte] & 0xFF];
            }
            outputs[iOutput][iByte] = (char) value;
        }
    }
}

void InputByteOutputTableCodingLoop::codeSomeShards(
        CVec2D &matrixRows,
        CVec2D &inputs,
        int inputCount,
        CVec2D &outputs,
        int outputCount,
        int offset, int byteCount) {

    CVec2D table = Galois::MULTIPLICATION_TABLE;
    {
        int iInput = 0;
        CVec &inputShard = inputs[iInput];
        for (int iByte = offset; iByte < offset + byteCount; iByte++) {
            char inputByte = inputShard[iByte];
            CVec &multTableRow = table[inputByte & 0xFF];
            for (int iOutput = 0; iOutput < outputCount; iOutput++) {
                CVec &outputShard = outputs[iOutput];
                CVec &matrixRow = matrixRows[iOutput];
                outputShard[iByte] = multTableRow[matrixRow[iInput] & 0xFF];
            }
        }
    }
    for (int iInput = 1; iInput < inputCount; iInput++) {
        CVec &inputShard = inputs[iInput];
        for (int iByte = offset; iByte < offset + byteCount; iByte++) {
            char inputByte = inputShard[iByte];
            CVec &multTableRow = table[inputByte & 0xFF];
            for (int iOutput = 0; iOutput < outputCount; iOutput++) {
                CVec &outputShard = outputs[iOutput];
                CVec &matrixRow = matrixRows[iOutput];
                outputShard[iByte] ^= multTableRow[matrixRow[iInput] & 0xFF];
            }
        }
    }
}

void InputOutputByteTableCodingLoop::codeSomeShards(
        CVec2D &matrixRows,
        CVec2D &inputs,
        int inputCount,
        CVec2D &outputs,
        int outputCount,
        int offset, int byteCount) {

    CVec2D table = Galois::MULTIPLICATION_TABLE;

    {
        int iInput = 0;
        CVec &inputShard = inputs[iInput];
        for (int iOutput = 0; iOutput < outputCount; iOutput++) {
            CVec &outputShard = outputs[iOutput];
            CVec &matrixRow = matrixRows[iOutput];
            CVec &multTableRow = table[matrixRow[iInput] & 0xFF];
            for (int iByte = offset; iByte < offset + byteCount; iByte++) {
                outputShard[iByte] = multTableRow[inputShard[iByte] & 0xFF];
            }
        }
    }

    for (int iInput = 1; iInput < inputCount; iInput++) {
        CVec &inputShard = inputs[iInput];
        for (int iOutput = 0; iOutput < outputCount; iOutput++) {
            CVec &outputShard = outputs[iOutput];
            CVec &matrixRow = matrixRows[iOutput];
            CVec &multTableRow = table[matrixRow[iInput] & 0xFF];
            for (int iByte = offset; iByte < offset + byteCount; iByte++) {
                outputShard[iByte] ^= multTableRow[inputShard[iByte] & 0xFF];
            }
        }
    }
}

//bool InputOutputByteTableCodingLoop::checkSomeShards(
//        CVec2D &matrixRows,
//        CVec2D &inputs,
//        int inputCount,
//        CVec2D &toCheck,
//        int checkCount,
//        int offset,
//        int byteCount,
//        CVec tempBuffer) {
//
////    if (tempBuffer.empty()) {
////        return this->CodingLoop::checkSomeShards(matrixRows, inputs, inputCount,
////                                                 toCheck, checkCount, offset, byteCount,
////                                                 CVec(0));
////    }
//
//    // This is actually the code from OutputInputByteTableCodingLoop.
//    // Using the loops from this class would require multiple temp
//    // buffers.
//
//    CVec2D table = Galois::MULTIPLICATION_TABLE;
//    for (int iOutput = 0; iOutput < checkCount; iOutput++) {
//        CVec &outputShard = toCheck[iOutput];
//        CVec &matrixRow = matrixRows[iOutput];
//        {
//            int iInput = 0;
//            CVec &inputShard = inputs[iInput];
//            CVec &multTableRow = table[matrixRow[iInput] & 0xFF];
//            for (int iByte = offset; iByte < offset + byteCount; iByte++) {
//                tempBuffer[iByte] = multTableRow[inputShard[iByte] & 0xFF];
//            }
//        }
//        for (int iInput = 1; iInput < inputCount; iInput++) {
//            CVec &inputShard = inputs[iInput];
//            CVec &multTableRow = table[matrixRow[iInput] & 0xFF];
//            for (int iByte = offset; iByte < offset + byteCount; iByte++) {
//                tempBuffer[iByte] ^= multTableRow[inputShard[iByte] & 0xFF];
//            }
//        }
//        for (int iByte = offset; iByte < offset + byteCount; iByte++) {
//            if (tempBuffer[iByte] != outputShard[iByte]) {
//                return false;
//            }
//        }
//    }
//    return true;
//}

void OutputByteInputTableCodingLoop::codeSomeShards(
        CVec2D &matrixRows,
        CVec2D &inputs,
        int inputCount,
        CVec2D &outputs,
        int outputCount,
        int offset, int byteCount) {

    CVec2D table = Galois::MULTIPLICATION_TABLE;
    for (int iOutput = 0; iOutput < outputCount; iOutput++) {
        CVec &outputShard = outputs[iOutput];
        CVec &matrixRow = matrixRows[iOutput];
        for (int iByte = offset; iByte < offset + byteCount; iByte++) {
            int value = 0;
            for (int iInput = 0; iInput < inputCount; iInput++) {
                CVec &inputShard = inputs[iInput];
                CVec &multTableRow = table[matrixRow[iInput] & 0xFF];
                value ^= multTableRow[inputShard[iByte] & 0xFF];
            }
            outputShard[iByte] = (char)value;
        }
    }
}

void OutputInputByteTableCodingLoop::codeSomeShards(
        CVec2D &matrixRows,
        CVec2D &inputs,
        int inputCount,
        CVec2D &outputs,
        int outputCount,
        int offset, int byteCount) {

    CVec2D table = Galois::MULTIPLICATION_TABLE;
    for (int iOutput = 0; iOutput < outputCount; iOutput++) {
        CVec &outputShard = outputs[iOutput];
        CVec &matrixRow = matrixRows[iOutput];
        {
            int iInput = 0;
            CVec &inputShard = inputs[iInput];
            CVec &multTableRow = table[matrixRow[iInput] & 0xFF];
            for (int iByte = offset; iByte < offset + byteCount; iByte++) {
                outputShard[iByte] = multTableRow[inputShard[iByte] & 0xFF];
            }
        }
        for (int iInput = 1; iInput < inputCount; iInput++) {
            CVec &inputShard = inputs[iInput];
            CVec &multTableRow = table[matrixRow[iInput] & 0xFF];
            for (int iByte = offset; iByte < offset + byteCount; iByte++) {
                outputShard[iByte] ^= multTableRow[inputShard[iByte] & 0xFF];
            }
        }
    }
}

//bool OutputInputByteTableCodingLoop::checkSomeShards(
//        CVec2D &matrixRows,
//        CVec2D &inputs,
//        int inputCount,
//        CVec2D &toCheck,
//        int checkCount,
//        int offset,
//        int byteCount,
//        CVec tempBuffer) {
//
//    CVec2D table = Galois::MULTIPLICATION_TABLE;
//    for (int iOutput = 0; iOutput < checkCount; iOutput++) {
//        CVec &outputShard = toCheck[iOutput];
//        CVec &matrixRow = matrixRows[iOutput];
//        {
//            int iInput = 0;
//            CVec &inputShard = inputs[iInput];
//            CVec &multTableRow = table[matrixRow[iInput] & 0xFF];
//            for (int iByte = offset; iByte < offset + byteCount; iByte++) {
//                tempBuffer[iByte] = multTableRow[inputShard[iByte] & 0xFF];
//            }
//        }
//        for (int iInput = 1; iInput < inputCount; iInput++) {
//            CVec &inputShard = inputs[iInput];
//            CVec &multTableRow = table[matrixRow[iInput] & 0xFF];
//            for (int iByte = offset; iByte < offset + byteCount; iByte++) {
//                tempBuffer[iByte] ^= multTableRow[inputShard[iByte] & 0xFF];
//            }
//        }
//        for (int iByte = offset; iByte < offset + byteCount; iByte++) {
//            if (tempBuffer[iByte] != outputShard[iByte]) {
//                return false;
//            }
//        }
//    }
//
//    return true;
//}