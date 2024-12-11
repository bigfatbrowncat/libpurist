#include "Displays.h"
#include "Display.h"
#include "ModeResources.h"
#include "ModeConnector.h"
#include "exceptions.h"
#include <stdexcept>
#include <cassert>

bool Displays::setAllCrtcs() {
	// bool some_modeset_failed = false;
	// /* perform actual modesetting on each found connector+CRTC */
	// for (auto& iter : *this) {
	// 	if (!iter->isCrtcSet()) {
	// 		auto set_successfully = iter->setCrtc();
	// 		if (!set_successfully) {
	// 			some_modeset_failed = true;
	// 			fprintf(stderr, "cannot set CRTC for connector %u (%d): %m\n",
	// 				iter->getConnectorId(), errno);
	// 		}
	// 	}
	// }
	// return !some_modeset_failed;
	throw std::runtime_error("Unimplemented!!!");
}

void Displays::addNewlyConnectedToDrawingLoop() {
	/* redraw all outputs */
	for (auto& iter : *this) {
		iter->updateInDrawingLoop(*this->displayContentsFactory);
	}
}

Displays::iterator Displays::findDisplayOnConnector(const drmModeConnector *conn) {
	// Looking for the display on this connector
	for (auto iter = this->begin(); iter != this->end(); iter++) {
		if (iter->get()->getConnectorId() == conn->connector_id) {
			return iter;
		}
	}
	return end();	// Hasn't find any
}

int Displays::updateHardwareConfiguration()
{
	ModeResources modeRes(card);
	auto resources = modeRes.resources;

	/* iterate all connectors */
	for (unsigned int i = 0; i < resources->count_connectors; ++i) {
		/* get information for each connector */
		try {
			ModeConnector modeConnector(card, modeRes, i);
			auto connector = modeConnector.connector;

			auto display_iter = findDisplayOnConnector(connector);
			std::shared_ptr<Display> display = nullptr;
			bool new_display_connected = false;
			
			if (display_iter != end()) {
				// Display found
				display = *display_iter;
			} else {
				// create a new display
				auto cid = connector->connector_id;
				display = std::make_shared<Display>(card, *this, cid);
				new_display_connected = true;
			}

			/* call helper function to prepare this connector */
			int ret = display->setup(resources, connector);
			if (ret) {
				if (ret == -ENXIO || display_iter != end()) {
					this->remove(display);
				} else if (ret != -ENOENT) {
					errno = -ret;
					fprintf(stderr, "cannot setup device for connector %u:%u (%d): %m\n",
						i, resources->connectors[i], errno);
				}
				
				// if (display_iter != end()) {
				// 	this->erase(display_iter);
				// }
				
				display = nullptr;
				
				//drmModeFreeConnector(connector);
				continue;
			}

			/* free connector data and link device into global list */
			//drmModeFreeConnector(connector);
			
			if (new_display_connected) {
				assert(display);
				this->push_front(display);
			}

		} catch (const cant_get_connector_exception& e) {
			printf("%s\n", e.what());
			continue;
		}
	}

	return 0;
}

void Displays::setDisplayContentsFactory(std::shared_ptr<DisplayContentsFactory> factory) {
	this->displayContentsFactory = factory;
}


Displays::~Displays() {
	clear();
}
