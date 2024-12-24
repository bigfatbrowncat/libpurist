#include "ModeCrtc.h"
#include <cstring>
#include <purist/exceptions.h>


namespace purist::graphics {

ModeCrtc::ModeCrtc(const Card& card, uint32_t crtc_id)
		: cardFd(card.fd), crtcId(crtc_id) {
	crtc = drmModeGetCrtc(cardFd, crtcId);
	if (!crtc) {
		throw errcode_exception(-errno, "can't get Crtc for id " + std::to_string(crtc_id));
	}
}

ModeCrtc::~ModeCrtc() {
	drmModeFreeCrtc(const_cast<drmModeCrtcPtr>(crtc));
}


void ModeCrtc::set(const FrameBufferImpl& buf, const std::vector<uint32_t> connectors, const ModeModeInfo& info) {
	int ret = drmModeSetCrtc(cardFd, crtcId, buf.framebuffer_id, 0, 0,
			const_cast<uint32_t*>(connectors.data()), connectors.size(), const_cast<drmModeModeInfo*>(&info.info));

	if (ret) {
		throw errcode_exception(-errno, std::string("cannot assign crtc with framebuffer. ") + strerror(errno));
	}
}

}