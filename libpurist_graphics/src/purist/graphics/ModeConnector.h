#pragma once

#include "Card.h"
//#include "ModeModeInfo.h"
#include "ModeResources.h"

#include <purist/graphics/interfaces.h>

#include <xf86drmMode.h>

namespace purist::graphics {

class ModeModeInfo;

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

    const std::list<std::shared_ptr<ModeModeInfo>> getModes() const;
    uint32_t getConnectorId() const;

	virtual ~ModeConnector();

};

}