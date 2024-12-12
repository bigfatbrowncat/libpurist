#pragma once

#include "Card.h"
#include "ModeConnector.h"

#include <xf86drmMode.h>

class ModeEncoder {
private:
    // Forbidding object copying
    ModeEncoder(const ModeEncoder& other) = delete;
    ModeEncoder& operator = (const ModeEncoder& other) = delete;

	const drmModeEncoder *encoder;

public:
	explicit ModeEncoder(const ModeConnector& connector);
	ModeEncoder(const ModeConnector& connector, size_t index);

	uint32_t getCrtcId() const { return encoder->crtc_id; }
	bool isCrtcPossible(int j) const;

	virtual ~ModeEncoder();
};
