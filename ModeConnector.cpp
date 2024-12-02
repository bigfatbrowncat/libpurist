#include "ModeConnector.h"
#include "exceptions.h"

ModeConnector::ModeConnector(const Card& card, uint32_t connector_id) /*: card(card), connector_id(connector_id)*/ {
	connector = drmModeGetConnector(card.fd, connector_id);
	if (!connector) {
		throw cant_get_connector_exception(-errno, "cannot retrieve DRM connector " 
								+ std::to_string(connector_id));
	}
}

ModeConnector::ModeConnector(const Card& card, const ModeResources& resources, size_t index) 
	: ModeConnector(card, resources.resources->connectors[index]) { }

ModeConnector::~ModeConnector() {
	drmModeFreeConnector(const_cast<drmModeConnectorPtr>(connector));
}
