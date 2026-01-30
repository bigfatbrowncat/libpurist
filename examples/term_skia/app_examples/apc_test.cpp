#include <Resource.h>

#include "../base64.hpp"

#include "include/core/SkFontMgr.h"
#include "include/core/SkFontTypes.h"
#include "include/core/SkPictureRecorder.h"
#include "include/core/SkPicture.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkRect.h"
#include "include/core/SkGraphics.h"
#include "include/core/SkSurface.h"
#include "include/core/SkStream.h"
#include "include/core/SkData.h"
#include "include/core/SkFont.h"
#include "include/core/SkTypeface.h"
#include <chrono>
#include <include/core/SkFontMgr.h>
#include <include/ports/SkFontMgr_data.h>

#include <termios.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>

static sk_sp<SkFontMgr> globalFontMgr;
static std::shared_ptr<SkFont> font;

sk_sp<SkFontMgr> createFontMgr(const std::vector<Resource>& res) {
  std::vector<sk_sp<SkData>> dataVec;
  for (size_t i = 0; i < res.size(); i++) {
    std::cout << "Font data size: " << res[i].size() << std::endl;

    auto data = SkData::MakeUninitialized(res[i].size()); //MakeFromFileName("fonts/noto-sans/NotoSans-Regular.ttf");
    memcpy(data->writable_data(), res[i].data(), res[i].size());

    if (data != nullptr) {
      dataVec.push_back(data);
    }
  }

  SkSpan<sk_sp<SkData>> dataSpan(dataVec);

  sk_sp<SkFontMgr> fontMgr = SkFontMgr_New_Custom_Data(dataSpan); //SkFontMgr_New_FontConfig(nullptr);
  
  if (fontMgr != nullptr) {
    int families = fontMgr->countFamilies();
    std::cout << "Font families count: " << families << std::endl;
  }

  return fontMgr;
}


sk_sp<SkTypeface> getTypefaceByName(const std::string& name) {
  auto tf = globalFontMgr->matchFamilyStyle(name.c_str(), SkFontStyle());
  if (tf == nullptr) {
    throw std::runtime_error(std::string("No typeface found: ") + name);
  }
  return tf;
}


// A function that contains the drawing commands to be recorded
void draw_commands(SkCanvas* canvas, int width, int height, int v) {
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SK_ColorRED);
    paint.setStyle(SkPaint::kFill_Style);


    // 3. Draw a circle
    float centerX = width / 2.0f;
    float centerY = height / 2.0f;
    float radius = 100.0f;
    canvas->drawCircle(centerX, centerY, radius, paint);

    // 4. Draw some text
    paint.setColor(SK_ColorRED);
    //paint.setTextSize(SkIntToScalar(24));
    SkString message("Hello Skia!");
    
    SkPaint paint2;
    paint.setColor(SK_ColorWHITE);
    canvas->drawSimpleText(message.c_str(), message.size(), SkTextEncoding::kUTF8, centerX - 80, centerY + v, *font, paint2);
}

sk_sp<SkPicture> record_picture(int width, int height, int v) {
    SkPictureRecorder recorder;
    SkCanvas* recordingCanvas = recorder.beginRecording(SkRect::MakeIWH(width, height));
    draw_commands(recordingCanvas, width, height, v);
    return recorder.finishRecordingAsPicture();
}

size_t draw_picture(sk_sp<SkPicture> picture) {
  SkDynamicMemoryWStream wStream;
  picture->serialize(&wStream);

  // 2. Convert the stream to SkData
  sk_sp<SkData> data = wStream.detachAsData();

  // 3. Copy the data into a char array
  std::vector<char> charArray(data->size());
  memcpy(charArray.data(), data->data(), data->size());

  std::string command(charArray.begin(), charArray.end()); //"This is an APC command";

  auto encoded = base64::to_base64(command);

  std::string apc_sequence = "\033_<skpicture>" + encoded + "</skpicture>\033\\";

  {
    struct termios term, original;

    // 1. Get current terminal settings
    if (tcgetattr(STDIN_FILENO, &original) == -1) {
        perror("tcgetattr");
        exit(1);
    }
    term = original;

    // 2. Disable ECHO (and optionally ICANON for raw input)
    term.c_lflag &= ~ECHO; 
    // term.c_lflag &= ~(ICANON | ECHO); // Use this to also disable canonical mode

    // 3. Apply new settings immediately
    if (tcsetattr(STDIN_FILENO, TCSANOW, &term) == -1) {
        perror("tcsetattr");
        exit(1);
    }

    std::cout << apc_sequence << std::flush;

    // 4. Restore original settings
    if (tcsetattr(STDIN_FILENO, TCSANOW, &original) == -1) {
        perror("tcsetattr restore");
        exit(1);
    }
  }
  return encoded.size();
}

// Terminal Application Program Command example
int main() {
  std::cout << std::endl;

  auto time = std::chrono::high_resolution_clock::now();

  SkGraphics::Init();

  int width = 1280;
  int height = 720;

  globalFontMgr = createFontMgr({
      LOAD_RESOURCE(fonts_noto_sans1_NotoSans_Regular_ttf),
  });

  sk_sp<SkTypeface> tf = getTypefaceByName("Noto Sans");

  SkScalar fontSize = 1;
  font = std::make_shared<SkFont>(tf, fontSize);
  font->setHinting(SkFontHinting::kNone);
  font->setSize(32);

  const int VC = 100;
  float fps_vals[VC];
  int fps_val_index = 0;

  float fps_avg = 0.f;
  
  for (int i = 0; ;i++) {

    int v = 30 * sin((float)i / 10);

    sk_sp<SkPicture> picture = record_picture(width, height, v);
    size_t pic_size = draw_picture(picture);

    auto new_time = std::chrono::high_resolution_clock::now();
    float usecs = std::chrono::duration_cast<std::chrono::microseconds>(new_time - time).count();

    fps_vals[fps_val_index] = 1000000 / usecs;
    fps_avg += fps_vals[fps_val_index] / VC;
    fps_val_index = (fps_val_index + 1) % VC;
    fps_avg -= fps_vals[fps_val_index] / VC;

    //for (int k=0; k<10; k++) { fps_avg += fps_vals[k]; }
    //fps_avg /= 10;

    std::cout << "Rendering picture with size: " << pic_size << " bytes, at " << std::fixed << std::setprecision(1) << fps_avg << " fps. Hit Ctrl+C to stop...      \r";

    time = new_time;
  }

  return 0;
}
