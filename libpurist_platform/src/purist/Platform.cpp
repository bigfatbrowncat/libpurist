#include <purist/Platform.h>
#include <purist/exceptions.h>
//#include <purist/graphics/DisplayContentsHandler.h>

//#include "graphics/Card.h"
//#include "input/Keyboards.h"
#include "global_init.h"

//#include <xkbcommon/xkbcommon.h>

#include <cassert>
#include <cstring>
#include <memory>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace purist {

Platform::Platform(bool enableOpenGL)
        : enableOpenGL(enableOpenGL) { 
    fix_ld_library_path();
}

void Platform::run(const std::vector<std::shared_ptr<DeviceClassProvider>>& dcProviders) {

    while (!stopPending) {
        // Building the list of fds
        std::vector<pollfd> fds;
        for (auto& dcp : dcProviders) {
            auto fds_p = dcp->getFds();
            fds.insert(fds.end(), fds_p.begin(), fds_p.end());
        }

        // Polling
        int ret = poll(fds.data(), fds.size(), 100);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            throw errcode_exception(-errno, "Couldn't poll for events");
        }

        // Processing every fd
        std::vector<pollfd>::iterator fds_iter = fds.begin();
        for (auto& dcp : dcProviders) {
            dcp->processFd(fds_iter);
        }
        
        // Updating the providers
        for (auto& dcp : dcProviders) {
            dcp->updateHardware();
        }
    }

    std::cout << "platform runloop ends." << std::endl;
}

void Platform::stop() {
    stopPending = true;
}

Platform::~Platform() {
    
}

}