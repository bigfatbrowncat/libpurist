#pragma once

#include "Card.h"

#include <xf86drmMode.h>

namespace purist::platform {

class ModeCrtc {
private:
    // Forbidding object copying
    ModeCrtc(const ModeCrtc& other) = delete;
    ModeCrtc& operator = (const ModeCrtc& other) = delete;

public:
	const drmModeCrtc *crtc;

	explicit ModeCrtc(const Card& card, uint32_t crtc_id);
	virtual ~ModeCrtc();
};

}