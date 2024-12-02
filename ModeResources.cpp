#include "ModeResources.h"
#include "exceptions.h"

ModeResources::ModeResources(const Card& card) /*: card(card)*/ {
	resources = drmModeGetResources(card.fd);
	if (!resources) {
		throw errcode_exception(-errno, "cannot retrieve DRM resources");
	}
}

ModeResources::~ModeResources() {
	drmModeFreeResources(const_cast<drmModeResPtr>(resources));
}
