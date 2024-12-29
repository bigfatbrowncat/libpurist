#pragma once

#include <cstdint>

namespace purist::input {

struct Modifiers {
    bool ctrl = false;
    bool alt = false;
    bool shift = false;

};

struct Leds {
    bool numLock = false;
    bool capsLock = false;
    bool scrollLock = false;
};

class Keyboard;

class KeyboardHandler {
public:
    virtual void onCharacter(Keyboard& kbd, uint32_t utf8CharCode) { }
    virtual void onKeyPress(Keyboard& kbd, uint32_t keysym, Modifiers mods, Leds leds, bool repeat) { }
    virtual void onKeyRelease(Keyboard& kbd, uint32_t keysym, Modifiers mods, Leds leds) { }
};

}
