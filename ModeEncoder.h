#pragma once

#include "Card.h"
#include "ModeConnector.h"

#include <xf86drmMode.h>

class ModeEncoder {
private:
    // Forbidding object copying
    ModeEncoder(const ModeEncoder& other) = delete;
    ModeEncoder& operator = (const ModeEncoder& other) = delete;

public:
	const drmModeEncoder *encoder;

	explicit ModeEncoder(const ModeConnector& connector);
	ModeEncoder(const ModeConnector& connector, size_t index);
	virtual ~ModeEncoder();
};
