#pragma once

#include "Card.h"

#include <xf86drmMode.h>

class ModeResources {
private:
    // Forbidding object copying
    ModeResources(const ModeResources& other) = delete;
    ModeResources& operator = (const ModeResources& other) = delete;

public:
	const drmModeRes *resources;

	ModeResources(const Card& card);
	virtual ~ModeResources();
};
