#include "CodingLoop.h"

bool CodingLoop::checkSomeShards(
        CVec2D &matrixRows,
        CVec2D &inputs,
        int inputCount,
        CVec2D &toCheck,
        int checkCount,
        int offset,
        int byteCount)
{
    // This is the loop structure for ByteOutputInput, which does not
    // require temporary buffers for checking.
    CVec2D table = Galois::MULTIPLICATION_TABLE;
    for (int iByte = offset; iByte < offset + byteCount; iByte++) {
        for (int iOutput = 0; iOutput < checkCount; iOutput++) {
            CVec &matrixRow = matrixRows[iOutput];
            int value = 0;
            for (int iInput = 0; iInput < inputCount; iInput++) {
                value ^= table[matrixRow[iInput] & 0xFF][inputs[iInput][iByte] & 0xFF];
            }
            if (toCheck[iOutput][iByte] != (char)value) {
                return false;
            }
        }
    }
    return true;
}