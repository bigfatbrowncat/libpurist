#include <purist/graphics/skia/SkiaOverlay.h>

#include <include/core/SkSurface.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>

#include <string>
#include <iostream>

namespace purist::graphics::skia {

void SkiaOverlay::createFontMgr() {
    fontMgr = SkFontMgr_New_FontConfig(nullptr);
    int families = fontMgr->countFamilies();
    std::cout << "Font families count: " << families << std::endl;
}

SkiaOverlay::SkiaOverlay() {
    createFontMgr();
}

sk_sp<SkTypeface> SkiaOverlay::getTypeface(const std::string& name) const {
  //auto tf = SkFontMgr::RefEmpty()->makeFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 0);  //ToolUtils::CreatePortableTypeface("serif", SkFontStyle());
  //auto tf = fontMgr->makeFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 0);  //ToolUtils::CreatePortableTypeface("serif", SkFontStyle());
  auto tf = fontMgr->matchFamilyStyle(/*"sans-serif"*/name.c_str(), SkFontStyle());
  if (tf == nullptr) {
    throw std::runtime_error(std::string("No typeface found") + name);
  }
  return tf;
}

}