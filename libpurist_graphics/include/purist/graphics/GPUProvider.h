#pragma once

#include <purist/Platform.h>
#include <purist/graphics/DisplayContentsHandler.h>

#include <memory>

namespace purist::graphics {

std::shared_ptr<DeviceClassProvider> createGPUProvider(
    std::shared_ptr<DisplayContentsHandler> displayContentsHandler, bool enableOpenGL);

}

//__attribute__((visibility("default")))
//extern "C" void * gbmint_get_backend(const void * gbm_core);
