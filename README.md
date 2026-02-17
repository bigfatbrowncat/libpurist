<style>
    h4 { font-size: 120%; margin: 0; padding: 0; font-weight: normal; }
    .c { font-size: 90%; margin-top: 10px; line-height: 1.3;}
</style>
# libpurist

A slim bare-linux (no XOrg/Wayland) graphics input/output library designed to be easy, transparent and portable. That includes binary portability of the user applivcations.

The main features supported so far:

* Dynamic displays connection/disconnection
* Multilingual text input (using `xkbcommon` library, check `text_input_skia` example)
* Plain graphics framebuffer output over `kmsdrm` (check `blinking_screen` example)
* Both OpenGL and CPU rasterization
* Vector graphics rendering with `skia` (check `term_skia` and `text_input_skia` examples)

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
        purist_graphics_skia["`<h4><b>libpurist</b> :: graphics :: skia</h4><div class='c'>Skia-dependent graphics API provider layer<div>`"]

        purist_graphics["`<h4><b>libpurist</b> :: graphics</h4><div class='c'>Basic EGL and CPU framebuffer graphics API provider layer</div>`"]
        
        purist_input["`<h4><b>libpurist</b> :: input</h4><div class='c'>Input API provider</div>`"]

        purist_graphics_skia --> purist_graphics
    end

    subgraph OS
        gl["<h4><b>mesa:</b> libegl + libgles</h4><div class='c'>The underlying system OpenGL<br/> implementation</div>"]
        kernel["<h4>Kernel</h4><div class='c'>Provides GPU<br/>and input devices</div>"]
    end
    
    subgraph libpurist dependencies
    
    purist_graphics_skia --> skia

    purist_graphics --> glvnd
    purist_graphics --> drm
    
    purist_input --> xkbcommon

    skia["<h4><b>skia</b> + <em>deps</em></h4><div class='c'>A popular and powerful<br/>vector 2D<br/>rendering engine</div>"]
    drm["<h4>libdrm + libgbm</h4><div class='c'>Controlling GPU,<br/> enumerating displays<br/>managing framebuffers</div>"]
    
    xkbcommon["<h4>libxkbcommon</h4><div class='c'>Processes keyboard input,<br/>converts scancodes<br/>to characters</div>"]

    glvnd["<h4>libglvnd</h4><div class='c'>A universal dynamic<br/>wrapper over<br/>OpenGL/GLES API</div>"]

    drm -->|/dev/dri/cardX| kernel
    xkbcommon -- /dev/input/... --> kernel
    
    glvnd -- Dynamic load and calls --> gl
    end 

```

## User requirements
To run the `libpurist` examples the user has to be in two groups: `input` and `video`

    $ sudo usermod -a -G input,video <username>


## Dependencies

These packages have to be installed for the building to succeed

    $ sudo apt install --no-install-recommends cmake libtool-bin libfreetype-dev libfontconfig-dev libegl-dev libgles-dev libjpeg-dev libwebp-dev libxkbcommon-dev libgbm-dev libdrm-dev libevdev-dev
