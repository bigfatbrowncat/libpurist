#include <purist/Platform.h>
#include <purist/exceptions.h>

#include "graphics/Card.h"
#include "input/Keyboard.h"
#include "input/Keyboards.h"

#include <xkbcommon/xkbcommon.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace purist {

Platform::Platform(bool enableOpenGL)
        : enableOpenGL(enableOpenGL) { }

static std::unique_ptr<graphics::Card> probeCard(bool enableOpenGL) {
    // Probing dri cards
    std::unique_ptr<graphics::Card> card;
    fs::path dri_path = "/dev/dri";
    std::string card_path;
    for (const auto & entry : fs::directory_iterator(dri_path)) {
        card_path = entry.path();
        if (card_path.find(std::string(dri_path / "card")) == 0) {
            try {
                card = std::make_unique<graphics::Card>(card_path, enableOpenGL);
                card->initialize();
            } catch (const errcode_exception& ex) {
                if (ex.errcode == EOPNOTSUPP) {
                    std::cout << "DRM not supported at " << card_path << std::endl;
                } else {
                    std::cout << "DRM can not be initialized at " << card_path << ": " << strerror(errno) << std::endl;
                }
                card = nullptr;
                continue;
            }
            break; // Success
        }
    }
    if (card == nullptr) {
        throw std::runtime_error("Can't find a card supporting DRM.");
    }
    std::cout << "Using videocard device: " << card_path << std::endl;
    return card;
}



void Platform::run(std::shared_ptr<graphics::DisplayContentsFactory> contentsFactory) {
    
    auto card = probeCard(enableOpenGL);

    auto keyboards = std::make_shared<purist::input::Keyboards>();
    keyboards->initialize();

    // Probing keyboards
    //auto kbd = probeKeyboard(ctx);

    card->setDisplayContentsFactory(contentsFactory);
    card->runDrawingLoop();

    printf("exiting\n");
}

Platform::~Platform() {
    
}

}