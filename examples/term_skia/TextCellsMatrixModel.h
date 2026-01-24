#pragma once

// Skia headers
#include <include/core/SkColor.h>

// C++ std headers
#include <cstdint>

typedef struct {
  int row;
  int col;
} TextCellsPos;

struct TextCell {
  std::string utf8;
  SkColor4f foreColor, backColor;
};

class TextCellsMatrixModel {
public:
    virtual TextCell getCell(int32_t row, int32_t col) = 0;
    virtual TextCellsPos getCursorPos() = 0;
};
