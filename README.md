# libpurist

A slim bare-linux (using no Xorg/Wayland) graphics input/output framework designed to be easy, transparent and portable. That includes binary portability of the user applications.

The main features supported so far:

* Dynamic displays connection/disconnection
* Both OpenGL and CPU rasterization
* Plain graphics framebuffer output over `kmsdrm` (check `blinking_screen` example)
* Multilingual text input using `xkbcommon` library (check `text_input_skia` example)
* Advanced vector graphics rendering with `skia` (check any `*_skia` examples)

The library is an ideal backend to build swift and graphically-rich applications for low-end systems like ARM64 single board computers. Tested thoroughly on:

* Raspberry Pi 4
* Banana Pi BPI-M7
* Intel Core i7-1065G7 CPU laptop

At every system of the listed above the library takes minimal resources, utilizes less than 5% of a single CPU core (in idle) and allows rendering of a UHD 3840x2160 @ 60 FPS (or multiple FullHD displays at once).

## General architecture
This is the general architecture diagram of libpurist library. 

```mermaid
---
config:
  flowchart:
    htmlLabels: false
    wrappingWidth: 450
    subGraphTitleMargin:
        top: 10
        bottom: 5
---
flowchart

    subgraph libpurist
        purist_graphics_skia["
            <div style="font-size:130%;"><b>libpurist</b> :: graphics :: skia</div>
            <div style='font-size:90%;line-height:1.2;margin:5px 0 0 0'>Skia-dependent graphics API provider layer<div>
        "]

        purist_graphics["
            <div style="font-size:130%;"><b>libpurist</b> :: graphics</div>
            <div style='font-size:90%;line-height:1.2;margin:5px 0 0 0'>Basic EGL and CPU framebuffer graphics API provider layer</div>
        "]
        
        purist_input["
            <div style="font-size:130%;"><b>libpurist</b> :: input</div>
            <div style='font-size:90%;line-height:1.2;margin:5px 0 0 0'>Input API provider</div>
        "]

        purist_graphics_skia --> purist_graphics
    end


    subgraph OS
        gl["
            <div style="font-size:130%;"><b>mesa:</b> libegl + libgles</div>
            <div style='font-size:90%;line-height:1.2;margin:5px 0 0 0'>The underlying system OpenGL<br/> implementation</div>
        "]
        kernel["
            <div style="font-size:130%;">Kernel</div>
            <div style='font-size:90%;line-height:1.2;margin:5px 0 0 0'>Provides GPU<br/>and input devices</div>
        "]
    end
    

    subgraph libpurist dependencies
    
        purist_graphics_skia --> skia

        purist_graphics --> glvnd
        purist_graphics --> drm
        
        purist_input --> xkbcommon

        skia["
            <div style="font-size:130%;"><b>skia</b> + <em>deps</em></div>
            <div style='font-size:90%;line-height:1.2;margin:5px 0 0 0'>A popular and powerful<br/>vector 2D<br/>rendering engine</div>
        "]

        drm["
            <div style="font-size:130%;">libdrm + libgbm</div>
            <div style='font-size:90%;line-height:1.2;margin:5px 0 0 0'>Controlling GPU,<br/> enumerating displays<br/>managing framebuffers</div>
        "]
        
        xkbcommon["
            <div style="font-size:130%;">libxkbcommon</div>
            <div style='font-size:90%;line-height:1.2;margin:5px 0 0 0'>Processes keyboard input,<br/>converts scancodes<br/>to characters</div>
        "]

        glvnd["
            <div style="font-size:130%;">libglvnd</div>
            <div style='font-size:90%;line-height:1.2;margin:5px 0 0 0'>A universal dynamic<br/>wrapper over<br/>OpenGL/GLES API</div>
        "]

        drm -->|/dev/dri/cardX| kernel
        xkbcommon -- /dev/input/... --> kernel
        
        glvnd -- Dynamic load and calls --> gl
    end 

```

The library consists of 3 top-level components that the user should interact with:

* **purist :: graphics** — Basic graphics API. Supports both GPU-backed GL rendering and CPU-backed pixel-wise framebuffer operation. Leaves all the details to the user. Use this one:
    - if you need direct FB access
    - if you brought your own GL-based rendering system
    - if you make an application for an extremely weak system that doesn't support Skia

* **purist :: graphics :: skia** — Skia-based wrapper around the aforementioned API. Provides easy, precise and effective 2D vector graphics rendering. Use this in most cases. 

* **purist :: input** — A keyboard input-processing API. Provides both scancode-based key handlers and advanced locale-based character handler.

## End-user requirements
To run any `libpurist`-based application (including the provided examples), the user has to be in two groups: `input` and `video`. To add them to these groups, do:

    $ sudo usermod -a -G input,video <username>


## Dependencies

These packages have to be installed for the building to succeed

    $ sudo apt install --no-install-recommends cmake libtool-bin libfreetype-dev libfontconfig-dev libegl-dev libgles-dev libjpeg-dev libwebp-dev libxkbcommon-dev libgbm-dev libdrm-dev libevdev-dev
