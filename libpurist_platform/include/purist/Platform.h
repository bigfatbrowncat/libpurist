#pragma once

//#include <purist/graphics/DisplayContentsHandler.h>
//#include <purist/input/KeyboardHandler.h>

#include <poll.h>

#include <memory>
#include <vector>

namespace purist {

// Provides all the devices of a specific class, 
// that should be treated the same way,
// like all keyboards or all GPUs.
class DeviceClassProvider {
public:
    // Returns the file descriptors for all the open devices
    virtual std::vector<pollfd> getFds() = 0;
    // This function should be called after the poll is done
    virtual void updateHardware() = 0;
    // This function takes iterator to the begin of the file descriptors
    // of a provider and has to be iterated to the end of them
    // by the implementation
    virtual void processFd(std::vector<pollfd>::iterator& iter) = 0;
};

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

    void run(const std::vector<std::shared_ptr<DeviceClassProvider>>& dcProviders);

    // void run(
    //     std::shared_ptr<graphics::DisplayContentsHandler> contents, 
    //     std::shared_ptr<input::KeyboardHandler> keyboardHandler);
    
    void stop();
};

}