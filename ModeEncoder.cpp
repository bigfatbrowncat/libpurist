#include "ModeEncoder.h"
#include "ModeConnector.h"
#include "exceptions.h"

ModeEncoder::ModeEncoder(const ModeConnector& connector) {
	encoder = drmModeGetEncoder(connector.card.fd, connector.connector->encoder_id);
	if (!encoder) {
		throw errcode_exception(-errno, "cannot retrieve encoder " + std::to_string(connector.connector->encoder_id));
	}
}

ModeEncoder::ModeEncoder(const ModeConnector& connector, size_t index) {
	encoder = drmModeGetEncoder(connector.card.fd, connector.connector->encoders[index]);
	if (!encoder) {
		throw errcode_exception(-errno, "cannot retrieve encoder " + std::to_string(index));
	}
}

ModeEncoder::~ModeEncoder() {
	drmModeFreeEncoder(const_cast<drmModeEncoderPtr>(encoder));
}
