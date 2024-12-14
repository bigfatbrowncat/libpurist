#include "ModeResources.h"
#include <purist/platform/exceptions.h>

namespace purist::platform {

ModeResources::ModeResources(const Card& card) {
	resources = drmModeGetResources(card.fd);
	if (!resources) {
		throw errcode_exception(-errno, "cannot retrieve DRM resources");
	}
}

ModeResources::~ModeResources() {
	drmModeFreeResources(const_cast<drmModeResPtr>(resources));
}

}