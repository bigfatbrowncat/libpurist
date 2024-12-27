#pragma once

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
public:

    Keyboard(const fs::path& node);
    bool initializeAndProbe(xkb_keymap *keymap, xkb_compose_table *compose_table);
    int getFd() const { return fd; }
    int read_keyboard(bool with_compose);
    virtual ~Keyboard();

    void runDrawingLoop() { }
};

}