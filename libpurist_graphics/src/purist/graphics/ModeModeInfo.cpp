#include "ModeModeInfo.h"

#include <purist/exceptions.h>

namespace purist::graphics {

ModeModeInfo::ModeModeInfo(const Card& card, const ModeConnector& conn, size_t index) : info(conn.connector->modes[index]) {
}

ModeModeInfo::~ModeModeInfo() {
}

bool operator == (const ModeModeInfo& mode1, const ModeModeInfo& mode2) {
    return 
        mode1.info.vdisplay == mode2.info.vdisplay &&
        mode1.info.hdisplay == mode2.info.hdisplay &&
        mode1.info.clock == mode2.info.clock;
}

}