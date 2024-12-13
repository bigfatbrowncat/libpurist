#pragma once

#include "interfaces.h"
#include <purist/platform/exceptions.h>
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
