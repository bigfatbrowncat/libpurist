#pragma once

#include "Card.h"

#include <cstddef>
#include <xf86drmMode.h>

class ModeResources {
private:
    // Forbidding object copying
    ModeResources(const ModeResources& other) = delete;
    ModeResources& operator = (const ModeResources& other) = delete;

	const drmModeRes *resources;

public:
	ModeResources(const Card& card);
    
    int getCountConnectors() const { return resources->count_connectors; }
    uint32_t getConnectorId(int index) const { return resources->connectors[index]; }

    int getCountCrtcs() const { return resources->count_crtcs; }
    uint32_t getCrtcId(int index) const { return resources->crtcs[index]; }

    virtual ~ModeResources();
};
