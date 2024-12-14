#include <purist/platform/Platform.h>
#include <purist/platform/exceptions.h>

#include "Card.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace purist::platform {

Platform::Platform(bool enableOpenGL)
        : enableOpenGL(enableOpenGL) { }

void Platform::run(std::shared_ptr<DisplayContentsFactory> contentsFactory) {
    // Probing dri cards
    std::unique_ptr<Card> ms;
    fs::path dri_path = "/dev/dri";
    std::string card_path;
    for (const auto & entry : fs::directory_iterator(dri_path)) {
        card_path = entry.path();
        if (card_path.find(std::string(dri_path / "card")) == 0) {
            try {
                ms = std::make_unique<Card>(card_path, enableOpenGL);
                ms->initialize();
            } catch (const errcode_exception& ex) {
                if (ex.errcode == EOPNOTSUPP) {
                    std::cout << "DRM not supported at " << card_path << std::endl;
                } else {
                    std::cout << "DRM can not be initialized at " << card_path << ": " << strerror(errno) << std::endl;
                }
                ms = nullptr;
                continue;
            }
            break; // Success
        }
    }
    if (ms == nullptr) {
        throw std::runtime_error("Can't find a card supporting DRM.");
    }
    std::cout << "Using " << card_path << std::endl;

    ms->setDisplayContentsFactory(contentsFactory);
    ms->runDrawingLoop();

    printf("exiting\n");
}

Platform::~Platform() {
    
}

}