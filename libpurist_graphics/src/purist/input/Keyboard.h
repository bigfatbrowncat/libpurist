#pragma once

//#include <evdevw.hpp>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>

#include <memory>
#include <list>
#include <set>
#include <filesystem>

namespace fs = std::filesystem;

namespace purist::input {

class Keyboard {
private:
    // Forbidding object copying
    Keyboard(const Keyboard& other) = delete;
    Keyboard& operator = (const Keyboard& other) = delete;

    fs::path node;
    int fd;
    struct xkb_state *state;
    struct xkb_compose_state *compose_state;

    static bool evdev_bit_is_set(const unsigned long *array, int bit);

    /* Some heuristics to see if the device is a keyboard. */
    static bool is_keyboard(int fd);
    
public:

    Keyboard(const fs::path& node);
    bool initializeAndProbe(xkb_keymap *keymap, xkb_compose_table *compose_table);

    virtual ~Keyboard();

    void runDrawingLoop() { }
};

}