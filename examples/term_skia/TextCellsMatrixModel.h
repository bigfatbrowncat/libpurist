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
  std::string utf8 = "";
  SkColor4f foreColor, backColor;
};

class TextCellsDataUpdate {
public:
  virtual std::optional<TextCell> getCell(int32_t row, int32_t col) = 0;
  virtual std::optional<TextCellsPos> getCursorPos() = 0;
  virtual std::optional<sk_sp<SkPicture>> getPicture() = 0;
  virtual int getPictureShiftedUpLines() = 0;
  virtual bool isBellSet() = 0;
  virtual std::optional<bool> isCursorVisible() = 0;
  virtual std::optional<bool> isCursorBlink() = 0;

  virtual ~TextCellsDataUpdate() = default;
};

class TextCellsMatrixModel {
public:
  virtual std::shared_ptr<TextCellsDataUpdate> getContentsUpdate() = 0;
  virtual ~TextCellsMatrixModel() = default;
};