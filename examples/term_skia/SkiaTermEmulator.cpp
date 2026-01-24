#include "SkiaTermEmulator.h"

// Skia headers
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkFont.h>
#include <include/core/SkFontMetrics.h>

void SkiaTermEmulator::refreshRect(int start_row, int start_col, int end_row, int end_col) {
    for (auto& mat_pair : screenUpdateMatrices) {
        auto& matrix = *(mat_pair.second);
        for (int row = start_row; row < end_row; row++) {
            for (int col = start_col; col < end_col; col++) {
                matrix(row, col) = framebuffersCount;
            }
        }
    }
}


sk_sp<SkImage> SkiaTermEmulator::drawCells(int col_min, int col_max, int row, //int row_min, int row_max, 
                int buffer_width, int buffer_height,
                std::shared_ptr<pgs::SkiaOverlay> skiaOverlay) {

    SkScalar epsilon = 0.0005f; // This very small value is added to the skale factor 
                                // to make sure that the images will not have any gaps between them

    SkScalar kx = ((SkScalar)buffer_width / (col_max - col_min) + epsilon) / font_width;
    SkScalar ky = ((SkScalar)buffer_height + epsilon) / font_height;

    //SkColor4f color = SkColor4f::FromColor(SkColorSetRGB(128, 128, 128));
    //SkColor4f bgcolor = SkColor4f::FromColor(SkColorSetRGB(0, 0, 0));

    UErrorCode status = U_ZERO_ERROR;

    sk_sp<SkImage> letter_image = nullptr;
    
    /*if (!texture)*/ {
        /*for (int row = row_min; row < row_max; row++)*/ {
            row_key rk;
            rk.width = buffer_width;
            rk.height = buffer_height;
            for (int col = col_min; col < col_max; col++) {
                std::string utf8 = "";
                TextCell cell = model->getCell(row, col);

                utf8 = cell.utf8;


                litera_key litkey { utf8, buffer_width, buffer_height, cell.foreColor.toSkColor(), cell.backColor.toSkColor() };
                rk.push_back(litkey);
            }

            if (!typesettingBox.exists(rk)) {
                //static int cache_misses = 0;
                //std::cout << "cache miss: " << cache_misses++ << std::endl;
                //if (letter_surface == nullptr) {
                    
                    if (letter_surfaces.empty()) {
                        letter_surfaces.push_back(skiaOverlay->getSkiaSurface()->makeSurface(
                            SkImageInfo::MakeN32Premul(((uint32_t)buffer_width) ,   // * (col_max - col_min)
                                                                ((uint32_t)buffer_height))
                        ));
                        //std::cout << "Added new letter surface. In the cache: " << typesettingBox.size() << std::endl;
                    }
                    
                    sk_sp<SkSurface> letter_surface = nullptr;
                    letter_surface = letter_surfaces.back();
                    letter_surfaces.pop_back();

                //}

                auto& letter_canvas = *letter_surface->getCanvas();

                letter_canvas.scale(kx, ky);

                for (int col = col_min; col < col_max; col++) {
                    int c = col - col_min;
                    auto& letter_key = rk[c];

                    SkRect bgrect = { 
                        (float)c * font_width, 
                        0.0f, 
                        (float)(c + 1) * font_width,
                        (float)font_height
                    };
                    SkPaint bgpt(SkColor4f::FromColor(letter_key.bgcolor));
                    bgpt.setStyle(SkPaint::kFill_Style);
                    letter_canvas.drawRect(bgrect, bgpt);
                }

                for (int col = col_min; col < col_max; col++) {
                    int c = col - col_min;
                    auto& letter_key = rk[c];

                    // TODO TTF_SetFontStyle(font, style);
                    //std::cout << utf8.c_str();
                    auto& utf8 = letter_key.utf8;

                    SkRect bgrect = { 
                        (float)c * font_width, 
                        0.0f, 
                        (float)(c + 1) * font_width,
                        (float)font_height
                    };
                    letter_canvas.save();
                    letter_canvas.clipRect(bgrect);
                    letter_canvas.drawString(utf8.c_str(),
                                                c * font_width, font_height - font_descent, *font, 
                                                SkPaint(SkColor4f::FromColor(letter_key.fgcolor)));
                    letter_canvas.restore();
                }

                letter_image = letter_surface->makeImageSnapshot();

                std::optional<std::pair<row_key, SurfaceAndImage>> returned_back = typesettingBox.put(rk, { letter_surface, letter_image });
                if (returned_back.has_value()) {
                    auto& ret_surf = returned_back.value().second.surface;
                    auto ret_cnv = ret_surf->getCanvas();
                    ret_cnv->restoreToCount(0);
                    ret_cnv->resetMatrix();
                    //ret_cnv->clear(SK_ColorTRANSPARENT);
                    letter_surfaces.push_back(ret_surf);
                }

            } else {
                letter_image = typesettingBox.get(rk).image;
                assert(letter_image->width() == buffer_width && letter_image->height() == buffer_height);
            }
        }
    }
    return letter_image;
}

void SkiaTermEmulator::drawIntoSurface(std::shared_ptr<pg::Display> display, 
                        std::shared_ptr<pgs::SkiaOverlay> skiaOverlay, 
                        int width, int height, SkCanvas& canvas, bool refreshed) {

    if (framebuffersCount < display->getFramebuffersCount()) {
        // Setting how many framebuffers we should redraw at once
        framebuffersCount = display->getFramebuffersCount();
    }

    if (screenUpdateMatrices.find(display->getConnectorId()) == screenUpdateMatrices.end()) {
        screenUpdateMatrices[display->getConnectorId()] = std::make_shared<cells<unsigned char>>(rows, cols);
        screenUpdateMatrices[display->getConnectorId()]->fill(framebuffersCount);
    }
    
    auto& matrix = *(screenUpdateMatrices[display->getConnectorId()]);
    
    SkScalar w = width, h = height;


    const SkScalar gray_hsv[] { 0.0f, 0.0f, 0.7f };
    auto color = SkColor4f::FromColor(SkHSVToColor(255, gray_hsv));
    //auto paint_gray = SkPaint(color_gray);

    const SkScalar black_hsv[] { 0.0f, 0.0f, 0.0f };
    auto bgcolor = SkColor4f::FromColor(SkHSVToColor(255, black_hsv));



    assert(matrix.getCols() % divider == 0);
    int part_width = matrix.getCols() / divider;


    int buffer_width = w / divider;
    int cols_per_buffer = cols / divider;

    // Patching buffer_width so that a single character consists of an integer number of pixels
    bool presize_horizontal = true;
    int rem = buffer_width % cols_per_buffer;
    float hscale = 1.0f;
    if (presize_horizontal && rem > 0) {
            buffer_width += cols_per_buffer - rem;
            hscale = (w / divider) / (float)buffer_width;
    }

    int buffer_height = h / matrix.getRows();

    // Checking if the height is even
    float vscale = 1.0f;
    if (matrix.getRows() * buffer_height == h) {
        vscale = 1.0f; // no scale
    } else {
        // Let the height be a bit over the necessary size, 
        // because shrinking the screen buffer looks beautifullier than stretching
        buffer_height ++;
        vscale = h / (static_cast<float>(buffer_height) * matrix.getRows());
    }
    
    canvas.scale(hscale, vscale);
    
    SkScalar font_width_scaled = w / matrix.getCols() / hscale;

    auto cursor_pos = model->getCursorPos();

    {
        //std::lock_guard<std::mutex> lock(matrixMutex);
        for (int col_part = 0; col_part < divider; col_part++) {
            for (int row = 0; row < matrix.getRows(); row++) {
                // Checking for the damage
                bool damaged = false;
                
                for (int col = part_width * col_part; col < part_width * (col_part + 1); col++) {
                    if (matrix(row, col) > 0) { matrix(row, col) -= 1; damaged = true; }
                    
                    // Because the cursor is blinking, we are always repainting it
                    if (col == cursor_pos.col && row == cursor_pos.row) { damaged = true; }
                }

                //if (damaged) std::cout << "damaged" << std::endl;

                if (damaged) {
                    // Drawing
                    auto cells_image = drawCells(part_width * col_part,
                            part_width * (col_part + 1), row, 
                            buffer_width, buffer_height, skiaOverlay);

                    //canvas.drawImage(cells_image, buffer_width * col_part, buffer_height * row);

                    //void drawImageRect(const SkImage*, const SkRect& dst, const SkSamplingOptions&);
                    if (presize_horizontal) {
                        SkRect dst = {
                            (float)buffer_width * col_part, // * hscale, 
                            (float)buffer_height * row,
                            (float)buffer_width * (col_part + 1), // * hscale,
                            (float)buffer_height * (row + 1),
                        };
                        SkSamplingOptions so { SkFilterMode::kLinear };
                        canvas.drawImageRect(cells_image, dst, so);
                    } else {
                        canvas.drawImage(cells_image, buffer_width * col_part, buffer_height * row);
                    }

                }
            }
        }
    }
    
    // draw cursor
    //VTermScreenCell cell;
    //vterm_screen_get_cell(screen, cursor_pos, &cell);

    auto cur_color = SkColor4f::FromColor(SkColorSetRGB(128, 128, 128));
    // if (VTERM_COLOR_IS_INDEXED(&cell.fg)) {
    //     vterm_screen_convert_color_to_rgb(screen, &cell.fg);
    // }

    TextCell cell = model->getCell(cursor_pos.row, cursor_pos.col);
    //if (VTERM_COLOR_IS_RGB(&cell.fg)) {
        cur_color = cell.foreColor; // SkColor4f::FromColor(SkColorSetRGB(cell.fg.rgb.red, cell.fg.rgb.green, cell.fg.rgb.blue));
    //}

    SkRect rect = { 
        (float)cursor_pos.col * font_width_scaled, 
        (float)cursor_pos.row * buffer_height, 
        (float)(cursor_pos.col + 1) * font_width_scaled, 
        (float)(cursor_pos.row + 1) * buffer_height
    };

    if (cursorVisible) {
        auto cur_time = std::chrono::system_clock::now();
        auto sec_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(cur_time.time_since_epoch());
        if (cursorBlink) {
            cursorPhase = (float)(sec_since_epoch.count() % cursorBlinkPeriodMSec) / cursorBlinkPeriodMSec;
        } else {
            cursorPhase = 0.0f;
        }
        float cursor_alpha = 0.5 * cos(2 * M_PI * (float)cursorPhase) + 0.5;
        auto cursor_color = SkColor4f({ 
            cur_color.fR,
            cur_color.fG,
            cur_color.fB,
            cursor_alpha });
        auto cursor_paint = SkPaint(cursor_color);

        //SkPaint cur_paint(cur_color);
        cursor_paint.setStyle(SkPaint::kFill_Style);
        canvas.drawRect(rect, cursor_paint);
    }

    if (ringingFramebuffers > 0) {
        canvas.clear(color);
        for (auto& mat_pair : screenUpdateMatrices) {
            auto& matrix = *(mat_pair.second);
            matrix.fill(framebuffersCount);
        }
        ringingFramebuffers -= 1;
    }
}


SkiaTermEmulator::SkiaTermEmulator(uint32_t _rows, uint32_t _cols) 
    : rows(_rows), cols(_cols), typesettingBox(_rows * divider * 4) {

    // Checking the arguments
    if (_cols == 0 || _rows == 0) {
        throw std::logic_error(std::string("Columns and rows count has to be positive"));
    }
    if (_cols % divider != 0) { 
        throw std::logic_error(std::string("Columns count ") + std::to_string(_cols) + std::string(" should be divideable by te number of substrings ") + std::to_string(divider));
    }
}

void SkiaTermEmulator::setTypeface(sk_sp<SkTypeface> typeface) {
    this->typeface = typeface;

    SkScalar fontSize = 1;
    font = std::make_shared<SkFont>(typeface, fontSize);
    font->setHinting(SkFontHinting::kNone);

    SkFontMetrics mets;
    font->getMetrics(&mets);

    auto font_height_patch = 0.05;

    auto font_width_patch = -0.01;
    auto font_descent_patch = 0.0;

    font_width = mets.fAvgCharWidth + font_width_patch;   // This is the real character width for monospace
    font_height = fontSize + font_height_patch;           // Applying font height patch
    font_descent = mets.fDescent + font_descent_patch;    // Patching the descent for the specific font (here is for Hack)

    //typesettingBox.clear(); TODO!!!
    
}

SkiaTermEmulator::~SkiaTermEmulator() { 

}
