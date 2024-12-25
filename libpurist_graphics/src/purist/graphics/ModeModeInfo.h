#pragma once

#include "Card.h"
#include "ModeConnector.h"
#include <purist/graphics/interfaces.h>

#include <xf86drmMode.h>

namespace purist::graphics {

class ModeModeInfo : public Mode {
private:
    // Forbidding object copying
    ModeModeInfo(const ModeModeInfo& other) = delete;
    ModeModeInfo& operator = (const ModeModeInfo& other) = delete;

public:
	const drmModeModeInfo info;

    uint32_t getWidth() const override { return info.hdisplay; }
    uint32_t getHeight() const override { return info.vdisplay; }
    float getFreq() const override { return info.clock * 1000.0f / (info.htotal * info.vtotal); }

	ModeModeInfo(const Card& card, const purist::graphics::ModeConnector& conn, size_t index);
	virtual ~ModeModeInfo();

};

bool operator == (const ModeModeInfo& mode1, const ModeModeInfo& mode2);

}