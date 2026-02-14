#include "TermSubprocess.h"
#include "SkiaTermRenderer.h"
#include "VTermWrapper.h"

// libpurist headers
#include <purist/graphics/skia/DisplayContentsSkia.h>
#include <purist/graphics/Display.h>
#include <purist/graphics/Mode.h>
#include <purist/input/KeyboardHandler.h>
#include <purist/Platform.h>
// #include <purist/graphics/skia/icu_common.h>
#include <Resource.h>

// Skia headers
#include <include/core/SkCanvas.h>

// C++ std headers
#include <memory>
#include <thread>
#include <iostream>

namespace p = purist;
namespace pg = purist::graphics;
namespace pi = purist::input;
namespace pgs = purist::graphics::skia;


int FPS = 60;

class TermDisplayContents : public pgs::SkiaDisplayContentsHandler, public pi::KeyboardHandler {
public:
    std::weak_ptr<p::Platform> platform;
    std::shared_ptr<TermSubprocess> subprocess;
    std::shared_ptr<VTermWrapper> vtermWrapper;
    std::shared_ptr<SkiaTermRenderer> termEmu;

    std::unique_ptr<std::thread> processThread;
       
    TermDisplayContents(std::weak_ptr<p::Platform> platform, uint32_t _rows, uint32_t _cols) 
            : platform(platform) {

        std::string prog = getenv("SHELL");
        subprocess = std::make_shared<TermSubprocess>(_rows, _cols, prog, std::vector<std::string> {"-"});
        vtermWrapper = std::make_shared<VTermWrapper>(_rows, _cols, subprocess);

        processThread = std::make_unique<std::thread>([&]() {
            while (vtermWrapper->processInputFromSubprocess()) { }
            std::cout << "I/O thread gracefully stopped." << std::endl;
        });

        termEmu = std::make_shared<SkiaTermRenderer>(_rows, _cols);
        termEmu->setModel(vtermWrapper);

    }

    void drawIntoSurface(std::shared_ptr<pg::Display> display, 
                        std::shared_ptr<pgs::SkiaOverlay> skiaOverlay, 
                        int width, int height, SkCanvas& canvas, bool refreshed) override {
        //vtermWrapper->processInputFromSubprocess();

        if (subprocess->isExited()) {
            platform.lock()->stop();
            return;
        }

        termEmu->drawIntoSurface(display, skiaOverlay, width, height, canvas, refreshed);
    }

    virtual ~TermDisplayContents() {
        processThread->join();
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
			if ((*mode)->getFreq() == FPS && (*mode)->getWidth() < 4000) {
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
        vtermWrapper->processCharacter(kbd, charCode, mods, leds);
    }

    void onKeyPress(pi::Keyboard& kbd, uint32_t keysym, pi::Modifiers mods, pi::Leds leds, bool repeat) override { 
        vtermWrapper->processKeyPress(kbd, keysym, mods, leds, repeat);
    }

    void onKeyRelease(pi::Keyboard& kbd, uint32_t keysym, pi::Modifiers mods, pi::Leds leds) override {

    }

};

void print_sizes() {
    std::cerr << "Valid sizes are: "
            << "tiny, petite, small, middle, large, grand, huge, colossal" << std::endl;
}

int main(int argc, char **argv)
{
    try {

        //const int rows = 12, cols = 40;       // Tiny    (3.33333)
        //const int rows = 25, cols = 80;       // Petite    (3.33333)
        int rows = 35, cols = 108;    // Small   (3.33333)
        //const int rows = 42, cols = 136;    // Middle  (3.4)
        //const int rows = 45, cols = 152;    // Large   (3.37777)
        //const int rows = 50, cols = 160;    // Grand    (3.2)
        //const int rows = 60, cols = 192;    // Huge    (3.2)
        //const int rows = 72, cols = 240;    // Gigantic  (3.33333)


        if (argc == 2) {
            auto s = std::string(argv[1]);
            if (s == "--help" || s == "-h") {
                std::cerr << "Usage: " << argv[0] << " [size]" << std::endl << std::endl;
                print_sizes();
                return 0;
            }


            if (s == "tiny" || s == "t") {
                rows = 24; cols = 80;
            } else if (s == "petite" || s == "p") {      // big for 6 inch
                rows = 28; cols = 96;
            } else if (s == "small" || s == "s") {       // small for 6 inch
                rows = 36; cols = 120;
            } else if (s == "middle" || s == "m") {
                rows = 38; cols = 128;
            } else if (s == "large" || s == "l") {
                rows = 48; cols = 160;
            } else if (s == "grand" || s == "g") {
                rows = 57; cols = 192;
            } else if (s == "huge" || s == "h") {
                rows = 72; cols = 240;
            } else if (s == "colossal" || s == "c") {
                rows = 76; cols = 256;
            } else {
                std::cerr << "Invalid option: " << s << std::endl << std::endl;
                print_sizes();
                return 1;
            }
        }

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
