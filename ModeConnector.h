#pragma once

#include "Card.h"
#include "ModeResources.h"

#include <xf86drmMode.h>

class ModeConnector {
public:
    const drmModeConnector *connector;

	ModeConnector(const Card& card, uint32_t connector_id);
    ModeConnector(const Card& card, const ModeResources& resources, size_t index);

	virtual ~ModeConnector();

};