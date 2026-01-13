#include <include/core/SkColor.h>

#include <string>

struct litera_key {
    std::string utf8;
    int width, height;
    SkColor fgcolor, bgcolor;

    bool operator == (const litera_key& other) const {
        return utf8 == other.utf8 &&
                width == other.width &&
                height == other.height &&
                fgcolor == other.fgcolor &&
                bgcolor == other.bgcolor;
    }

    bool operator < (const litera_key& other) const {
        if (utf8 < other.utf8) return true;
        else if (utf8 == other.utf8) {

            if (width < other.width) return true;
            else if (width == other.width) {

                if (height < other.height) return true;
                else if (height == other.height) {

                    if (fgcolor < other.fgcolor) return true;
                    else if (fgcolor == other.fgcolor) {

                        if (bgcolor < other.bgcolor) return true;
                    }
                }
            }
        }
        return false;                   
    }
};
