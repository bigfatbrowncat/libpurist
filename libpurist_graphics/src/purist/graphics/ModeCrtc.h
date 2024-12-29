#pragma once

#include "Card.h"
#include "ModeConnector.h"
#include "ModeModeInfo.h"
#include "FrameBufferImpl.h"

#include <xf86drmMode.h>

#include <vector>

namespace purist::graphics {

class ModeCrtc {
private:
    // Forbidding object copying
    ModeCrtc(const ModeCrtc& other) = delete;
    ModeCrtc& operator = (const ModeCrtc& other) = delete;

    int cardFd;
    uint32_t crtcId;
public:
	const drmModeCrtc *crtc;

	explicit ModeCrtc(const Card& card, uint32_t crtc_id);
	virtual ~ModeCrtc();

    void set(const FrameBufferImpl& buf, const std::vector<uint32_t> connectors, const ModeModeInfo& info);
};

}