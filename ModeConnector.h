#pragma once

#include "Card.h"
#include "ModeResources.h"

#include <xf86drmMode.h>

class ModeConnector {
private:
    // Forbidding object copying
    ModeConnector(const ModeConnector& other) = delete;
    ModeConnector& operator = (const ModeConnector& other) = delete;

public:
    const Card& card;
    const drmModeConnector *connector;
    const uint32_t connector_id;

	ModeConnector(const Card& card, uint32_t connector_id);
    ModeConnector(const Card& card, const ModeResources& resources, size_t index);

	virtual ~ModeConnector();

};