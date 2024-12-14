#include "ModeConnector.h"
#include <purist/platform/exceptions.h>
#include <cassert>

namespace purist::platform {

ModeConnector::ModeConnector(const Card& card, uint32_t connector_id) : card(card), connector_id(connector_id) {
	connector = drmModeGetConnector(card.fd, connector_id);
	if (!connector) {
		throw cant_get_connector_exception(-errno, "cannot retrieve DRM connector " 
								+ std::to_string(connector_id));
	}
}

ModeConnector::ModeConnector(const Card& card, const ModeResources& resources, size_t index) 
	: ModeConnector(card, resources.getConnectorId(index)) { }

ModeConnector::~ModeConnector() {
	drmModeFreeConnector(const_cast<drmModeConnectorPtr>(connector));
}

uint32_t ModeConnector::getConnectorId() const { 
	assert(connector->connector_id == connector_id);
	return connector->connector_id; 
}

}