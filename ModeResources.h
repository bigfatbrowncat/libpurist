#pragma once

#include "Card.h"

#include <xf86drmMode.h>

class ModeResources {
public:
	const drmModeRes *resources;

	ModeResources(const Card& card);
	virtual ~ModeResources();
};
