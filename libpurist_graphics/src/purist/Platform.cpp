#include <purist/Platform.h>
#include <purist/exceptions.h>
#include <purist/graphics/DisplayContentsHandler.h>

#include "graphics/Card.h"
#include "input/Keyboard.h"
#include "input/Keyboards.h"
#include "global_init.h"

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

static std::unique_ptr<graphics::Card> probeCard(std::shared_ptr<graphics::DisplayContentsHandler> contents, bool enableOpenGL) {
    // Probing dri cards
    std::unique_ptr<graphics::Card> card;
    fs::path dri_path = "/dev/dri";
    std::string card_path;
    for (const auto & entry : fs::directory_iterator(dri_path)) {
        card_path = entry.path();
        if (card_path.find(std::string(dri_path / "card")) == 0) {
            try {
                card = std::make_unique<graphics::Card>(card_path, enableOpenGL);
                card->initialize(contents);
            } catch (const errcode_exception& ex) {
                if (ex.errcode == EOPNOTSUPP) {
                    std::cout << "DRM not supported at " << card_path << std::endl;
                } else {
                    std::cout << "DRM can not be initialized at " << card_path << ": " << ex.what() << std::endl;
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



void Platform::run(std::shared_ptr<graphics::DisplayContentsHandler> contentsFactory, 
                   std::shared_ptr<input::KeyboardHandler> keyboardHandler) {
    
    fix_ld_library_path();

    auto keyboards = std::make_shared<purist::input::Keyboards>();
    keyboards->initialize();

    auto card = probeCard(contentsFactory, enableOpenGL);

    uint32_t keyboards_poll_counter = 0;
    uint32_t polls_between_updates = 100;
    while (!stopPending) {
        // Rebuilding the list of fds
        auto fds = keyboards->getFds();

        // Appending the videocard fd
        fds.push_back(pollfd {
            .fd = card->fd,
            .events = POLLIN,
            .revents = 0
        });

        int ret = poll(fds.data(), fds.size(), 100);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            throw errcode_exception(-errno, "Couldn't poll for events");
        }

		if (keyboards_poll_counter % polls_between_updates == 0) {
			keyboards->updateHardwareConfiguration(keyboardHandler);
            keyboards_poll_counter = 0;
        }

        // Processing the keyboards
        auto fds_iter = fds.begin();
        for (; fds_iter != fds.end() - 1; fds_iter++) {
            keyboards->processFd(fds_iter);
        }
        
        // Processing the videocard
        card->processFd(fds_iter);

        keyboards_poll_counter++;
    }


    // Probing keyboards
    //auto kbd = probeKeyboard(ctx);

    //card->runDrawingLoop();

    printf("exiting\n");
}

void Platform::stop() {
    stopPending = true;
}

Platform::~Platform() {
    
}

}