#pragma once

#include <purist/Platform.h>
#include <purist/graphics/DisplayContentsHandler.h>

#include <memory>

namespace purist::graphics {

std::shared_ptr<DeviceClassProvider> createGPUProvider(
    std::shared_ptr<DisplayContentsHandler> displayContentsHandler, bool enableOpenGL);

}