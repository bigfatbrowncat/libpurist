#include <purist/Platform.h>
#include <purist/exceptions.h>
#include <purist/graphics/interfaces.h>

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

static std::unique_ptr<graphics::Card> probeCard(std::shared_ptr<graphics::DisplayContentsFactory> factory, bool enableOpenGL) {
    // Probing dri cards
    std::unique_ptr<graphics::Card> card;
    fs::path dri_path = "/dev/dri";
    std::string card_path;
    for (const auto & entry : fs::directory_iterator(dri_path)) {
        card_path = entry.path();
        if (card_path.find(std::string(dri_path / "card")) == 0) {
            try {
                card = std::make_unique<graphics::Card>(card_path, enableOpenGL);
                card->initialize(factory);
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



void Platform::run(std::shared_ptr<graphics::DisplayContentsFactory> contentsFactory, 
                   std::shared_ptr<input::KeyboardHandler> keyboardHandler) {
    
    auto keyboards = std::make_shared<purist::input::Keyboards>();
    keyboards->initialize(keyboardHandler);

    auto card = probeCard(contentsFactory, enableOpenGL);

    auto fds = keyboards->getFds();

    // Appending the videocard fd
    fds.push_back(pollfd {
        .fd = card->fd,
        .events = POLLIN,
        .revents = 0
    });


    bool terminate = false;
    while (!terminate) {
        int ret = poll(fds.data(), fds.size(), -1);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            throw errcode_exception(-errno, "Couldn't poll for events");
        }

        // Processing the keyboards
        auto fds_iter = fds.begin();
        for (; fds_iter != fds.end() - 1; fds_iter++) {
            keyboards->processFd(fds_iter);
        }
        
        // Processing the videocard
        card->processFd(fds_iter);
    }


    // Probing keyboards
    //auto kbd = probeKeyboard(ctx);

    //card->runDrawingLoop();

    printf("exiting\n");
}

Platform::~Platform() {
    
}

}