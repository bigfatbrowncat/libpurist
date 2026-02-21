#pragma once

#include <purist/input/KeyboardHandler.h>

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
    static Modifiers modifiersFromKeymap(xkb_keymap *keymap, xkb_state *state, xkb_keycode_t keycode, xkb_consumed_mode consumed_mode) {
        Modifiers res;
        for (xkb_mod_index_t mod = 0; mod < xkb_keymap_num_mods(keymap); mod++) {
            if (xkb_state_mod_index_is_active(state, mod,
                                            XKB_STATE_MODS_EFFECTIVE) <= 0)
                continue;

            std::string mod_name = xkb_keymap_mod_get_name(keymap, mod);
            if (mod_name == "Mod1") res.alt = true;
            if (mod_name == "Shift") res.shift = true;
            if (mod_name == "Control") res.ctrl = true;

            // if (xkb_state_mod_index_is_consumed2(state, keycode, mod,
            //                                     consumed_mode))
            //     printf("-%s ", xkb_keymap_mod_get_name(keymap, mod));
            // else
            //     printf("%s ", xkb_keymap_mod_get_name(keymap, mod));
        }
        return res;
    }

    static Leds ledsFromKeymap(xkb_keymap *keymap, xkb_state *state) {
        Leds res;
        for (xkb_led_index_t led = 0; led < xkb_keymap_num_leds(keymap); led++) {
            if (xkb_state_led_index_is_active(state, led) == 1) {
                std::string led_name = xkb_keymap_led_get_name(keymap, led);

                if (led_name == "Caps Lock") res.capsLock = true;
                if (led_name == "Num Lock") res.numLock = true;
                if (led_name == "Scroll Lock") res.scrollLock = true;
            }
        }
        return res;
    }


    /* The meaning of the input_event 'value' field. */
    enum {
        KEY_STATE_RELEASE = 0,
        KEY_STATE_PRESS = 1,
        KEY_STATE_REPEAT = 2,
    };

    // Forbidding object copying
    Keyboard(const Keyboard& other) = delete;
    Keyboard& operator = (const Keyboard& other) = delete;

    fs::path node;
    int fd;
    struct xkb_state *state = nullptr;
    struct xkb_compose_state *compose_state = nullptr;
    int evdev_offset = 8;
    enum xkb_consumed_mode consumed_mode = XKB_CONSUMED_MODE_XKB;

    static bool evdev_bit_is_set(const unsigned long *array, int bit);

    /* Some heuristics to see if the device is a keyboard. */
    static bool is_keyboard(int fd);
    
    void process_event(uint16_t type, uint16_t code, int32_t value, bool with_compose);
    void tools_print_keycode_state(const char *prefix,
                          struct xkb_state *state,
                          struct xkb_compose_state *compose_state,
                          xkb_keycode_t keycode,
                          enum xkb_consumed_mode consumed_mode);
    void tools_print_state_changes(enum xkb_state_component changed);

    std::shared_ptr<KeyboardHandler> keyboardHandler;

public:

    Keyboard(const fs::path& node);
    bool initializeAndProbe(xkb_keymap *keymap, xkb_compose_table *compose_table, std::shared_ptr<KeyboardHandler> keyboardHandler);
    int getFd() const { return fd; }
    
    // Returns true if it succeeded, 
    // false if the keyboard was disconnected, 
    // throws exception in other cases
    bool read_keyboard(bool with_compose);
    
    const fs::path& getNode() const { return node; }
    virtual ~Keyboard();
};

}