#pragma once

#include <memory>
#include <list>

namespace purist::graphics {

class Mode;
class TargetSurface;
class Display;

/*
    Implemented on the user side.
    The user implementation 
*/
class DisplayContentsHandler {
public:
    virtual ~DisplayContentsHandler() = default;

    /*
        The user implementation should return an iterator pointing to the desired graphics mode. 
        The standard implementation returns the first mode in the list (the highest resolution / refreshing rate)
    */
    virtual std::list<std::shared_ptr<Mode>>::const_iterator chooseMode(const std::list<std::shared_ptr<Mode>>& modes) {
        return modes.begin();
    }

    /*
        The user implementation should draw the contents of the screen into the specified target
    */
    virtual void drawIntoBuffer(std::shared_ptr<Display> display, std::shared_ptr<TargetSurface> target) = 0;
};

}