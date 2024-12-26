#pragma once

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>

#include "Keyboard.h"

namespace purist::input {

class Keyboards : private std::list<std::shared_ptr<Keyboard>> {
private:
    // Forbidding object copying
    Keyboards(const Keyboards& other) = delete;
    Keyboards& operator = (const Keyboards& other) = delete;

    xkb_context* ctx = nullptr;
    const char *keymap_path = nullptr;
    struct xkb_keymap *keymap = nullptr;

public:
    Keyboards();
    void initialize();

    virtual ~Keyboards();

    void runDrawingLoop() { }
};

}