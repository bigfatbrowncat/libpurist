#pragma once

#include "interfaces.h"

class Entry {
private:
    // Forbidding object copying
    Entry(const Entry& other) = delete;
    Entry& operator = (const Entry& other) = delete;

    bool enableOpenGL;

public:
    Entry(bool enableOpenGL = true);

    void run(std::shared_ptr<DisplayContentsFactory> contentsFactory);
    ~Entry();
};
