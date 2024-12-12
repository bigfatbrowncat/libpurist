#pragma once

#include <memory>
#include <list>
#include <set>

#include <gbm.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>


class Display;
class Displays;


class Card {
private:
    // Forbidding object copying
    Card(const Card& other) = delete;
    Card& operator = (const Card& other) = delete;

    gbm_device *gbmDevice = nullptr;

    int init_gbm(int fd, uint32_t width, uint32_t height);
    int init_gl(void);
public:
    EGLDisplay glDisplay;
    EGLConfig glConfig;
    EGLContext glContext;
    
    const int fd;
    const bool enableOpenGL = true;
    const std::shared_ptr<Displays> displays;

    Card(const char *node, bool enableOpenGL);
    virtual ~Card();

    void runDrawingLoop();

    gbm_device* getGBMDevice() const { return gbmDevice; }
};
