#pragma once

#include "interfaces.h"
#include "exceptions.h"

#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>

namespace purist::platform {

class Platform {
private:
    // Forbidding object copying
    Platform(const Platform& other) = delete;
    Platform& operator = (const Platform& other) = delete;

    bool enableOpenGL;

public:
    Platform(bool enableOpenGL = true);

    void run(std::shared_ptr<DisplayContentsFactory> contentsFactory);
    ~Platform();
};

}