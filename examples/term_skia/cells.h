#pragma once

#include <stdexcept>

template <typename T> class cells {
    T* buf;
    int rows, cols;
public:
    cells(int _rows, int _cols) : rows(_rows), cols(_cols) {
        buf = new T[cols * rows];
    }
    cells(int _rows, int _cols, const T& value) : rows(_rows), cols(_cols) {
        buf = new T[cols * rows];
        fill(value);
    }
    ~cells() {
        delete[] buf;
    }
    void fill(const T& by) {
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < cols; col++) {
                buf[cols * row + col] = by;
            }
        }
    }
    inline T& operator()(int row, int col) {
        if (row < 0 || col < 0 || row >= rows || col >= cols) throw std::runtime_error("invalid position");
        return buf[cols * row + col];
    }

    void setRect(int row_top, int rows_count, int col_left, int cols_count, const T& value) {
        for (int j = row_top; j < row_top + rows_count; j++) {
            for (int i = col_left; i < col_left + cols_count; i++) {
                buf[cols * j + i] = value;
            }
        }
    }

    int getRows() const { return rows; }
    int getCols() const { return cols; }
};
