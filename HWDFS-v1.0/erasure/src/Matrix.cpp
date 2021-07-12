#include "Matrix.h"
#include "Galois.h"

Matrix::Matrix(int initRows, int initColumns) {
    this->rows = initRows;
    this->columns = initColumns;
    this->data = CVec2D(initRows);
    for(int i = 0; i < initRows; i++) {
        this->data[i] = CVec(initColumns);
    }
}

Matrix::Matrix(CVec2D initData) {
    this->rows = initData.size();
    this->columns = initData[0].size();
    this->data = initData;
}

Matrix Matrix::identity(int size) {
    Matrix result(size, size);
    for(int i = 0; i < size; i++) {
        result.set(i, i, 1);
    }
    return result;
}

int Matrix::getColumns() {
    return this->columns;
}

int Matrix::getRows() {
    return this->rows;
}

char Matrix::get(int r, int c) {
    if(r < 0 || rows <= r) {
        cout << "Row index out of range: " << r << endl;
        throw exception(logic_error("Row index out of range"));
    }
    if(c < 0 || columns <= c) {
        cout << "Column index out of range: " << r << endl;
        throw exception(logic_error("Column index out of range"));
    }
    return data[r][c];
}

void Matrix::set(int r, int c, char value) {
    if(r < 0 || rows <= r) {
        cout << "Row index out of range: " << r << endl;
        throw exception(logic_error("Row index out of range"));
    }
    if(c < 0 || columns <= c) {
        cout << "Column index out of range: " << r << endl;
        throw exception(logic_error("Column index out of range"));
    }
    data[r][c] = value;
}

bool Matrix::operator==(Matrix &matrix) {
    if(this->getRows() != matrix.getRows()
        || this->getColumns() != matrix.getColumns()) {
        return false;
    } else {
        for(int i = 0; i < this->getRows(); i++) {
            for(int j = 0; j < this->getColumns(); j++){
                if(this->get(i, j) != matrix.get(i, j)) {
                    return false;
                }
            }
        }
    }
    return true;
}

Matrix Matrix::times(Matrix right) {
    if(getColumns() != right.getRows()) {
        cout << "Columns on left (" << getColumns()
             << ") is different than rows on right (" << right.getRows()
             << ")!" << endl;
        throw exception(logic_error("Columns on left is different than rows on right!"));
    }

    Matrix result(getRows(), right.getColumns());
    for(int r = 0; r < getRows(); r++) {
        for(int c = 0; c < right.getColumns(); c++) {
            char value = 0;
            for(int i = 0; i < getColumns(); i++) {
                value ^= Galois::multiply(get(r, i), right.get(i, c));
            }
            result.set(r, c, value);
        }
    }
    return result;
}

Matrix Matrix::augment(Matrix right) {
    if(getRows() != right.getRows()) {
        throw exception(logic_error("Matrices don't have the same number of rows!"));
    }
    Matrix result(getRows(), getColumns() + right.getColumns());
    for(int r = 0; r < getRows(); r++){
        for(int c = 0; c < getColumns(); c++){
            result.set(r, c, get(r, c));
        }
        for(int c = 0; c < right.getColumns(); c++) {
            result.set(r, getColumns() + c, right.get(r, c));
        }
    }
    return result;
}

Matrix Matrix::submatrix(int rmin, int cmin, int rmax, int cmax) {
    Matrix result(rmax - rmin, cmax - cmin);
    for(int r = rmin; r < rmax; r++) {
        for(int c = cmin; c < cmax; c++) {
            result.set(r - rmin, c - cmin, get(r, c));
        }
    }
    return result;
}

CVec Matrix::getRow(int row) {
    CVec result(getColumns());
    for(int c = 0; c < columns; c++) {
        result[c] = get(row, c);
    }
    return result;
}

void Matrix::swapRows(int r1, int r2) {
    if (r1 < 0 || rows <= r1 || r2 < 0 || rows <= r2) {
        throw exception(logic_error("Row index out of range!"));
    }
    CVec tmp = data[r1];
    data[r1] = data[r2];
    data[r2] = tmp;
}

Matrix Matrix::invert() {
    if(rows != columns) {
        throw exception(logic_error("Only square matrices can be inverted!"));
    }

    Matrix work = augment(identity(rows));
    work.gaussianElimination();
    return work.submatrix(0, rows, columns, columns * 2);
}

void Matrix::gaussianElimination() {
    for(int r = 0; r < rows; r++) {
        if(data[r][r] == (char)0) {
            for(int rowBelow = r + 1; rowBelow < rows; rowBelow++) {
                if(data[rowBelow][r] != 0) {
                    swapRows(r, rowBelow);
                    break;
                }
            }
        }

        if(data[r][r] == (char)0) {
            throw exception(logic_error("Matrix is singular!"));
        }

        if(data[r][r] != (char)1) {
            char scale = Galois::divide((char)1, data[r][r]);
            for(int c = 0; c < columns; c++) {
                data[r][c] = Galois::multiply(data[r][c], scale);
            }
        }

        for(int rowBelow = r + 1; rowBelow < rows; rowBelow++) {
            if(data[rowBelow][r] != (char)0) {
                char scale = data[rowBelow][r];
                for(int c = 0; c < columns; c++) {
                    data[rowBelow][c] ^= Galois::multiply(scale, data[r][c]);
                }
            }
        }
    }

    for(int d = 0; d < rows; d++) {
        for(int rowAbove = 0; rowAbove < d; rowAbove++) {
            if(data[rowAbove][d] != (char)0) {
                char scale = data[rowAbove][d];
                for(int c = 0; c < columns; c++) {
                    data[rowAbove][c] ^= Galois::multiply(scale, data[d][c]);
                }
            }
        }
    }
}