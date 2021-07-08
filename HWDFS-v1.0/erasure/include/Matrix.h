#pragma once
#include <iostream>
#include <stdexcept>
#include <string>
#include "Shared.h"
using namespace std;

class Matrix {
private:
    int rows;
    int columns;
    CVec2D data;

    void gaussianElimination();

public:
    Matrix() {}
    Matrix(int initRows, int initColumns);
    Matrix(CVec2D initData);
    static Matrix identity(int size);
    int getRows();
    int getColumns();
    char get(int r, int c);
    void set(int r, int c, char value);
    Matrix times(Matrix right);
    Matrix augment(Matrix right);
    Matrix submatrix(int rmin, int cmin, int rmax, int cmax);
    CVec getRow(int row);
    void swapRows(int r1, int r2);
    Matrix invert();

    bool operator==(Matrix &matrix);
};
