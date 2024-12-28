#pragma once

#include "exceptions.h"

#include "graphics/interfaces.h"
#include "input/interfaces.h"

#include <memory>

#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>

namespace purist {

class Platform {
private:
    // Forbidding object copying
    Platform(const Platform& other) = delete;
    Platform& operator = (const Platform& other) = delete;

    bool enableOpenGL;

public:
    Platform(bool enableOpenGL = true);

    void run(
        std::shared_ptr<graphics::DisplayContentsFactory> contentsFactory, 
        std::shared_ptr<input::KeyboardHandler> keyboardHandler);
    ~Platform();
};

}