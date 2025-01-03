#include "include/core/SkTypeface.h"
#include <purist/graphics/skia/SkiaOverlay.h>

#include <include/core/SkSurface.h>
#include <include/core/SkData.h>
#include <include/core/SkSpan.h>
#include <include/core/SkFontStyle.h>
#include <include/core/SkTypeface.h>

// #ifdef __APPLE__
// #  include <include/ports/SkFontMgr_mac_ct.h>
// #else
// #  include <include/ports/SkFontMgr_fontconfig.h>
// #endif

//#include <src/ports/SkFontMgr_custom.h>
//#include <include/ports/SkFontMgr_directory.h>
#include <include/ports/SkFontMgr_data.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>

#include <string>
#include <iostream>
#include <list>

namespace purist::graphics::skia {

void SkiaOverlay::createFontMgr(const std::vector<Resource>& res) {
  //Resource text = LOAD_RESOURCE(frag_glsl);
  
  for (size_t i = 0; i < res.size(); i++) {
    auto data = SkData::MakeWithCopy(res[i].data(), res[i].size()); //MakeFromFileName("fonts/noto-sans/NotoSans-Regular.ttf");
    dataVec.push_back(data);
  }

  SkSpan<sk_sp<SkData>> dataSpan(dataVec);

  fontMgr = SkFontMgr_New_Custom_Data(dataSpan); //SkFontMgr_New_FontConfig(nullptr);

  //fontMgr = SkFontMgr_New_Custom_Directory(fontDirectory.c_str());
    
  int families = fontMgr->countFamilies();
  std::cout << "Font families count: " << families << std::endl;
}

SkiaOverlay::SkiaOverlay() {
  //createFontMgr(fontDirectory);
}

sk_sp<SkTypeface> SkiaOverlay::getTypefaceByName(const std::string& name) const {
  //auto tf = SkFontMgr::RefEmpty()->makeFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 0);  //ToolUtils::CreatePortableTypeface("serif", SkFontStyle());
  //auto tf = fontMgr->makeFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 0);  //ToolUtils::CreatePortableTypeface("serif", SkFontStyle());
  auto tf = fontMgr->matchFamilyStyle(name.c_str(), SkFontStyle());
  if (tf == nullptr) {
    throw std::runtime_error(std::string("No typeface found: ") + name);
  }
  return tf;
}
//skia::textlayout::TextStyle
std::list<SkString> SkiaOverlay::getAllFontFamilyNames() const {
  // Listing the families
  std::list<SkString> familyNames;
  for (size_t i = 0; i < fontMgr->countFamilies(); i++) {
    SkString name;
    fontMgr->getFamilyName(i, &name);
    familyNames.push_back(name);
  }
  return familyNames;
}

sk_sp<SkTypeface> SkiaOverlay::getTypefaceForCharacter(SkUnichar unichar, const std::vector<SkString>& fontFamilies, const SkFontStyle& style) const {
  // Looking for a typeface suporting this character
  for (const SkString& fn : fontFamilies) {
    sk_sp<SkTypeface> tf = fontMgr->matchFamilyStyle(fn.c_str(), style);
    SkGlyphID gi = tf->unicharToGlyph(unichar);
    if (gi != 0) {
      return tf;
    }
  }
  return nullptr;
}


}