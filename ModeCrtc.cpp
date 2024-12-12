#include "ModeCrtc.h"
#include "exceptions.h"

ModeCrtc::ModeCrtc(const Card& card, uint32_t crtc_id) {
	crtc = drmModeGetCrtc(card.fd, crtc_id);
	if (!crtc) {
		throw errcode_exception(-errno, "can't get Crtc for id " + std::to_string(crtc_id));
	}
}

ModeCrtc::~ModeCrtc() {
	drmModeFreeCrtc(const_cast<drmModeCrtcPtr>(crtc));
}
