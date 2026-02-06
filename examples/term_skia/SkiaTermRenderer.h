#pragma once

#include "TextCellsMatrixModel.h"

#include "lru_cache.h"
#include "cells.h"
#include "row_key.h"

// libpurist headers
#include <purist/graphics/skia/DisplayContentsSkia.h>
#include <purist/graphics/Display.h>
// #include <purist/graphics/Mode.h>
// #include <purist/input/KeyboardHandler.h>
// #include <purist/Platform.h>
#include <purist/graphics/skia/icu_common.h>

// Skia headers
#include <include/core/SkSurface.h>
#include <include/core/SkImage.h>
#include <include/core/SkFont.h>

// xkbcommon headers
#include <xkbcommon/xkbcommon-keysyms.h>

// C++ std headers
#include <map>
#include <memory>
#include <cmath>
#include <mutex>
#include <vector>
#include <cassert>
#include <optional>

namespace p = purist;
namespace pg = purist::graphics;
namespace pgs = purist::graphics::skia;


class SkiaTermRenderer {
private:
    struct SurfaceAndImage {
        sk_sp<SkSurface> surface;
        sk_sp<SkImage> image;
    };

    float cursorPhase = 0;
    uint32_t cursorBlinkPeriodMSec = 300;

    sk_sp<SkTypeface> typeface;
    std::shared_ptr<SkFont> font;
    SkScalar font_width;
    SkScalar font_height;
    SkScalar font_descent;
    uint32_t ringingFramebuffers = 0;
    
    bool cursorVisible = true;
    bool cursorBlink = true;
    TextCellsPos cursor_pos { 0, 0 };
    cells<TextCell> text_cells;
    sk_sp<SkPicture> picture;
    int picture_shifted_up_lines = 0;

    uint32_t rows, cols;
    int divider = 4;

    lru_cache<row_key, SurfaceAndImage> typesettingBox;
    std::vector<sk_sp<SkSurface>> letter_surfaces;
    
    sk_sp<SkSurface> graphic_layer;
    sk_sp<SkSurface> text_layer;
    
    std::map<uint32_t, std::shared_ptr<cells<unsigned char>>> screenUpdateMatrices;  // The key is the display connector id

    uint32_t framebuffersCount = 0;

    std::shared_ptr<TextCellsMatrixModel> model;

    sk_sp<SkImage> drawCells(int col_min, int col_max, int row, //int row_min, int row_max, 
                   int buffer_width, int buffer_height,
                   std::shared_ptr<pgs::SkiaOverlay> skiaOverlay);

    void drawGraphicsLayer(std::shared_ptr<pgs::SkiaOverlay> skiaOverlay, 
                      int width, int height, int row_height, 
                      std::shared_ptr<TextCellsDataUpdate> modelUpdate);
    
    void drawTextLayer(std::shared_ptr<pgs::SkiaOverlay> skiaOverlay, 
                       cells<unsigned char>& matrix,
                       int width, int height, int row_height, 
                       std::shared_ptr<TextCellsDataUpdate> modelUpdate);

public:
    SkiaTermRenderer(uint32_t _rows, uint32_t _cols);

    void setModel(std::shared_ptr<TextCellsMatrixModel> model) {
        this->model = model;
    }
    void setTypeface(sk_sp<SkTypeface> typeface);
    void drawIntoSurface(std::shared_ptr<pg::Display> display, 
                         std::shared_ptr<pgs::SkiaOverlay> skiaOverlay, 
                         int width, int height, SkCanvas& canvas, bool refreshed);
    
    void refreshRect(int start_row, int start_col, int end_row, int end_col);
    void setCursorVisible(bool value) { cursorVisible = value; }
    void setCursorBlink(bool value) { cursorBlink = value; }
    void bellBlink() { ringingFramebuffers = framebuffersCount; }

    virtual ~SkiaTermRenderer();
};
