#pragma once

#include <memory>
#include <list>
#include <set>

class Display;
class Displays;

class Card {
    friend class Display;
    friend class Displays;

public:
    const int fd;
    const std::shared_ptr<Displays> displays;

    Card(const char *node);
    virtual ~Card();

    void runDrawingLoop();
};
