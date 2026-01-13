#include "TermSubprocess.h"
#include "SkiaTermEmulator.h"
#include "lru_cache.h"
#include "cells.h"
#include "row_key.h"

// libpurist headers
#include <purist/graphics/skia/DisplayContentsSkia.h>
#include <purist/graphics/Display.h>
#include <purist/graphics/Mode.h>
#include <purist/input/KeyboardHandler.h>
#include <purist/Platform.h>
#include <purist/graphics/skia/icu_common.h>
#include <Resource.h>

// Skia headers
#include <include/core/SkCanvas.h>

// C++ std headers
#include <memory>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <stdexcept>
#include <mutex>
#include <vector>
#include <cassert>
#include <chrono>
#include <optional>

namespace p = purist;
namespace pg = purist::graphics;
namespace pi = purist::input;
namespace pgs = purist::graphics::skia;


// For FullHD resolution:
//const int rows = 12, cols = 40;       // Toy    (3.33333)
//const int rows = 25, cols = 80;       // Tiny    (3.33333)
const int rows = 35, cols = 108;    // Small   (3.33333)
//const int rows = 42, cols = 136;    // Middle  (3.4)
//const int rows = 45, cols = 152;    // Large   (3.37777)
//const int rows = 50, cols = 160;    // Larger    (3.2)
//const int rows = 60, cols = 192;    // Huge    (3.2)
//const int rows = 72, cols = 240;    // Gigantic  (3.33333)

int FPS = 60;

class TermDisplayContents : public pgs::SkiaDisplayContentsHandler, public pi::KeyboardHandler {
public:
    std::weak_ptr<p::Platform> platform;
    std::shared_ptr<TermSubprocess> subprocess;
    std::shared_ptr<SkiaTermEmulator> termEmu;
       
    TermDisplayContents(std::weak_ptr<p::Platform> platform, uint32_t _rows, uint32_t _cols) 
            : platform(platform) {

        std::string prog = getenv("SHELL");
        subprocess = std::make_shared<TermSubprocess>(_rows, _cols, prog, std::vector<std::string> {"-"});

        termEmu = std::make_shared<SkiaTermEmulator>(_rows, _cols, subprocess);
        
    }

    void drawIntoSurface(std::shared_ptr<pg::Display> display, 
                        std::shared_ptr<pgs::SkiaOverlay> skiaOverlay, 
                        int width, int height, SkCanvas& canvas, bool refreshed) override {

        if (subprocess->isExited()) {
            platform.lock()->stop();
            return;
        }

        termEmu->drawIntoSurface(display, skiaOverlay, width, height, canvas, refreshed);
    }

    virtual ~TermDisplayContents() {
        termEmu = nullptr;
        subprocess = nullptr;
    }
    
    std::list<std::shared_ptr<pg::Mode>>::const_iterator chooseMode(std::shared_ptr<pgs::SkiaOverlay> skiaOverlay, const std::list<std::shared_ptr<pg::Mode>>& modes) override {

        if (skiaOverlay->getFontMgr() == nullptr) {
            skiaOverlay->createFontMgr({
                LOAD_RESOURCE(fonts_HackNerdFontMono_HackNerdFontMono_Regular_ttf)    // "Hack Nerd Font Mono"
                //LOAD_RESOURCE(fonts_DejaVuSansMono_DejaVuSansMono_ttf)
            });
        }

        auto typeface = skiaOverlay->getTypefaceByName("Hack Nerd Font Mono");
        termEmu->setTypeface(typeface);

        //auto typeface = skiaOverlay->getTypefaceByName("DejaVu Sans Mono");

        // Don't allow screens bigger than UHD
        // for (auto m = modes.begin(); m != modes.end(); m++) {
        //      if ((*m)->getWidth() < 4000 &&  
        //          (*m)->getHeight() < 4000) {
        //             return m;
        //          }
        // }
        // return modes.begin();

        std::list<std::shared_ptr<pg::Mode>>::const_iterator res = modes.begin();

        int max_width = 0;
		for(auto mode = modes.begin(); mode != modes.end(); mode++) {
			if ((*mode)->getFreq() == FPS && (*mode)->getWidth() < 2000) {
                if (max_width < (*mode)->getWidth()) {
                    res = mode;
                    max_width = (*mode)->getWidth();
                }
			}
		}

        FPS = (*res)->getFreq();

        return res;
    }

    pgs::DisplayOrientation chooseOrientation(std::shared_ptr<pg::Display> display, std::shared_ptr<pgs::SkiaOverlay> skiaOverlay) override {
        if (display->getMode().getHeight() > display->getMode().getWidth()) {
            // We are assumming that all the displays are horizontally oriented.
            // So if some of them have height > width, let's rotate it
            return pgs::DisplayOrientation::LEFT_VERTICAL;
        } else {
            return pgs::DisplayOrientation::HORIZONTAL;
        }
    }


    void onCharacter(pi::Keyboard& kbd, char32_t charCode, pi::Modifiers mods, pi::Leds leds) override { 
        termEmu->processCharacter(kbd, charCode, mods, leds);
    }

    void onKeyPress(pi::Keyboard& kbd, uint32_t keysym, pi::Modifiers mods, pi::Leds leds, bool repeat) override { 
        termEmu->processKeyPress(kbd, keysym, mods, leds, repeat);
    }

    void onKeyRelease(pi::Keyboard& kbd, uint32_t keysym, pi::Modifiers mods, pi::Leds leds) override {

    }

};


int main(int argc, char **argv)
{
    try {
        bool enableOpenGL = true;

        auto purist = std::make_shared<p::Platform>(enableOpenGL);

        auto contents = std::make_shared<TermDisplayContents>(purist, rows, cols);
        auto contents_handler_for_skia = std::make_shared<pgs::DisplayContentsHandlerForSkia>(contents, enableOpenGL);

        purist->run(contents_handler_for_skia, contents);

        return 0;

    } catch (const purist::errcode_exception& ex) {
        fprintf(stderr, "%s\n", ex.what());
        return ex.errcode;
    }
}
