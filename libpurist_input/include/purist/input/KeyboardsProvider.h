#pragma once

#include <purist/Platform.h>
#include <purist/input/KeyboardHandler.h>

#include <memory>

namespace purist::input {

std::shared_ptr<DeviceClassProvider> createKeyboardsProvider(std::shared_ptr<KeyboardHandler> keyboardHandler);

}