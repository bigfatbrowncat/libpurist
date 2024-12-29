#pragma once

#include <purist/graphics/interfaces.h>

#include <memory>
#include <list>
#include <vector>
#include <set>
#include <filesystem>

#include <gbm.h>
#include <poll.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace fs = std::filesystem;

namespace purist::graphics {

class Displays;

class Card {
private:
    // Forbidding object copying
    Card(const Card& other) = delete;
    Card& operator = (const Card& other) = delete;

    gbm_device *gbmDevice = nullptr;
    std::shared_ptr<Displays> displays;
    int counter = 0;

    int initGBM(int fd, uint32_t width, uint32_t height);
    int initGL();


public:
    EGLDisplay glDisplay;
    EGLConfig glConfig;
    EGLContext glContext;
    
    const int fd;
    const bool enableOpenGL = true;
    const fs::path node;

    Card(const fs::path& node, bool enableOpenGL);
    void initialize(std::shared_ptr<DisplayContentsHandler> factory);

    virtual ~Card();

    //void runDrawingLoop();
    void processFd(std::vector<pollfd>::iterator fds_iter);

    gbm_device* getGBMDevice() const { return gbmDevice; }

};

}