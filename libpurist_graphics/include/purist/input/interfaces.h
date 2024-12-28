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
    virtual void onCharacter(Keyboard& kbd, uint32_t utf8CharCode) = 0;
    virtual void onKeyPress(Keyboard& kbd, uint32_t keyCode, Modifiers mods, Leds leds) = 0;
    virtual void onKeyRepeat(Keyboard& kbd, uint32_t keyCode, Modifiers mods, Leds leds) = 0;
    virtual void onKeyRelease(Keyboard& kbd, uint32_t keyCode, Modifiers mods, Leds leds) = 0;
};

}
