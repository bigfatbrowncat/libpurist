#include "Displays.h"
#include "DisplayImpl.h"
#include "ModeResources.h"
#include "ModeConnector.h"

#include <purist/exceptions.h>

#include <stdexcept>
#include <cassert>

namespace purist::graphics {

void Displays::addNewlyConnectedToDrawingLoop() {
	/* redraw all outputs */
	for (auto& iter : *this) {
		iter->updateInDrawingLoop();
	}
}

Displays::iterator Displays::findDisplayOnConnector(const ModeConnector& conn) {
	// Looking for the display on this connector
	for (auto iter = this->begin(); iter != this->end(); iter++) {
		if (iter->get()->getConnectorId() == conn.connector->connector_id) {
			return iter;
		}
	}
	return end();	// Hasn't find any
}

std::shared_ptr<DisplayImpl> Displays::findDisplayConnectedToCrtc(uint32_t crtc_id) const {
	for (auto& disp : *this) {
		if (disp->getCrtcId() == crtc_id) {
			return disp;
		}
	}
	return nullptr;
}

int Displays::updateHardwareConfiguration()
{
	ModeResources modeRes(card);
	//auto resources = modeRes.resources;

	/* iterate all connectors */
	assert(modeRes.getCountConnectors() >= 0);
	for (unsigned int i = 0; i < (unsigned int)modeRes.getCountConnectors(); ++i) {
		/* get information for each connector */
		try {
			ModeConnector modeConnector(card, modeRes, i);
			//auto connector = modeConnector.connector;

			auto display_iter = findDisplayOnConnector(modeConnector);
			std::shared_ptr<DisplayImpl> display = nullptr;
			bool new_display_connected = false;
			
			if (display_iter != end()) {
				// Display found
				display = *display_iter;
			} else {
				// create a new display
				auto cid = modeConnector.getConnectorId(); // connector->connector_id;
				display = std::make_shared<DisplayImpl>(card, *this, cid, opengl);
				display->setContentsHandler(displayContents);
				new_display_connected = true;
			}

			/* call helper function to prepare this connector */
			int ret = display->setup(modeRes, modeConnector);
			if (ret) {
				if (ret == -ENXIO || display_iter != end()) {
					this->remove(display);
				} else if (ret != -ENOENT) {
					errno = -ret;
					fprintf(stderr, "cannot setup device for connector %u:%u (%d): %m\n",
						i, modeRes.getConnectorId(i), errno);	//  resources->connectors[i]
				}
				
				// If it is ENOENT
				display = nullptr;
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

bool Displays::empty() const {
	return std::list<std::shared_ptr<DisplayImpl>>::empty();
}

void Displays::clear() { 
	std::list<std::shared_ptr<DisplayImpl>>::clear(); 
}


void Displays::setDisplayContents(std::shared_ptr<DisplayContentsHandler> contents) {
	this->displayContents = contents;
}


Displays::~Displays() {
	clear();
}

}
