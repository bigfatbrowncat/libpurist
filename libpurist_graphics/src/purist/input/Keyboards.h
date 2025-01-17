#pragma once

#include "Keyboard.h"


#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>

#include <poll.h>

#include <vector>

namespace purist::input {

class Keyboards : private std::list<std::shared_ptr<Keyboard>> {
private:
    // Forbidding object copying
    Keyboards(const Keyboards& other) = delete;
    Keyboards& operator = (const Keyboards& other) = delete;

    xkb_context* ctx = nullptr;
    const char *keymap_path = nullptr;
    xkb_keymap *keymap = nullptr;
    xkb_compose_table *compose_table = NULL;
    bool with_compose = true;
public:
    Keyboards();
    void initialize();
    int loop();
    std::vector<pollfd> getFds();
    void processFd(std::vector<pollfd>::iterator fds_iter);
    void updateHardwareConfiguration(std::shared_ptr<input::KeyboardHandler> keyboardHandler);
    virtual ~Keyboards();

};

}