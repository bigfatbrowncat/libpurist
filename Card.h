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

public:
    struct _gbm {
        struct gbm_device *dev = nullptr;
        struct gbm_surface *surface;

        struct gbm_bo *bo;
        struct gbm_bo *previous_bo = NULL;

        uint32_t previous_fb;
        
        uint32_t handle;
        uint32_t pitch;
    } gbm;

    struct _gl {
        EGLDisplay display;
        EGLConfig config;
        EGLContext context;
        EGLSurface surface;
        GLuint program;
        GLint modelviewmatrix, modelviewprojectionmatrix, normalmatrix;
        GLuint vbo;
        GLuint positionsoffset, colorsoffset, normalsoffset;
    } gl;

    const int fd;
    const std::shared_ptr<Displays> displays;

    Card(const char *node);
    virtual ~Card();

    int init_gbm(int fd, uint32_t width, uint32_t height);
    int init_gl(void);
    void runDrawingLoop();
};
