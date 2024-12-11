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
    friend class Display;
    friend class Displays;

private:
    gbm_device *gbmDevice = nullptr;

public:

    EGLDisplay glDisplay;
    EGLConfig glConfig;
    EGLContext glContext;
    
    // GLuint program;
    // GLint modelviewmatrix, modelviewprojectionmatrix, normalmatrix;
    // GLuint vbo;
    // GLuint positionsoffset, colorsoffset, normalsoffset;

    const int fd;
    const bool enableOpenGL = true;
    const std::shared_ptr<Displays> displays;

    Card(const char *node, bool enableOpenGL);
    virtual ~Card();

    int init_gbm(int fd, uint32_t width, uint32_t height);
    int init_gl(void);
    void runDrawingLoop();

    gbm_device* getGBMDevice() const { return gbmDevice; }
};
