#pragma once

// Skia headers
#include <include/core/SkColor.h>
#include "include/core/SkPicture.h"

// C++ std headers
#include <cstdint>
#include <string>

struct TextCellsPos {
  int row;
  int col;
};

struct TextCell {
  std::string utf8;
  SkColor4f foreColor, backColor;
};

class TextCellsMatrixModel {
public:
    virtual TextCell getCell(int32_t row, int32_t col) = 0;
    virtual TextCellsPos getCursorPos() = 0;
    virtual sk_sp<SkPicture> getPicture() = 0;
};
