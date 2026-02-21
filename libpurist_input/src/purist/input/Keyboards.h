#pragma once

#include "Keyboard.h"


#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>

#include <poll.h>

#include <vector>

namespace purist::input {

class Keyboards : public DeviceClassProvider, private std::list<std::shared_ptr<Keyboard>> {
private:
    // Forbidding object copying
    Keyboards(const Keyboards& other) = delete;
    Keyboards& operator = (const Keyboards& other) = delete;

    xkb_context* ctx = nullptr;
    const char *keymap_path = nullptr;
    xkb_keymap *keymap = nullptr;
    xkb_compose_table *compose_table = NULL;
    bool with_compose = true;

    uint32_t keyboards_poll_counter = 0;
    uint32_t polls_between_updates = 100;

    std::shared_ptr<input::KeyboardHandler> keyboardHandler;

    void initialize();

public:
    Keyboards(std::shared_ptr<input::KeyboardHandler> keyboardHandler);
//    int loop();
    std::vector<pollfd> getFds() override;
    void processFd(std::vector<pollfd>::iterator& fds_iter) override;
    void updateHardware() override;
    virtual ~Keyboards();

};

}