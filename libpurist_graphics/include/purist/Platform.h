#pragma once

#include "exceptions.h"

#include <purist/graphics/DisplayContentsHandler.h>
#include <purist/input/interfaces.h>

#include <memory>

namespace purist {

class Platform {
private:
    // Forbidding object copying
    Platform(const Platform& other) = delete;
    Platform& operator = (const Platform& other) = delete;

    bool enableOpenGL;
    bool stopPending = false;
public:
    Platform(bool enableOpenGL = true);
    virtual ~Platform();

    void run(
        std::shared_ptr<graphics::DisplayContentsHandler> contents, 
        std::shared_ptr<input::KeyboardHandler> keyboardHandler);
    
    void stop();
};

}