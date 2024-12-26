#include <purist/Platform.h>
#include <purist/exceptions.h>

#include "graphics/Card.h"
#include "input/Keyboard.h"

#include <xkbcommon/xkbcommon.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace purist {

Platform::Platform(bool enableOpenGL)
        : enableOpenGL(enableOpenGL) { }

static std::unique_ptr<graphics::Card> probeCard(bool enableOpenGL) {
    // Probing dri cards
    std::unique_ptr<graphics::Card> card;
    fs::path dri_path = "/dev/dri";
    std::string card_path;
    for (const auto & entry : fs::directory_iterator(dri_path)) {
        card_path = entry.path();
        if (card_path.find(std::string(dri_path / "card")) == 0) {
            try {
                card = std::make_unique<graphics::Card>(card_path, enableOpenGL);
                card->initialize();
            } catch (const errcode_exception& ex) {
                if (ex.errcode == EOPNOTSUPP) {
                    std::cout << "DRM not supported at " << card_path << std::endl;
                } else {
                    std::cout << "DRM can not be initialized at " << card_path << ": " << strerror(errno) << std::endl;
                }
                card = nullptr;
                continue;
            }
            break; // Success
        }
    }
    if (card == nullptr) {
        throw std::runtime_error("Can't find a card supporting DRM.");
    }
    std::cout << "Using videocard device: " << card_path << std::endl;
    return card;
}

static std::unique_ptr<input::Keyboard> probeKeyboard(xkb_context *ctx) {



    const char *keymap_path = NULL;
    struct xkb_keymap *keymap = NULL;
    if (keymap_path) {
        FILE *file = fopen(keymap_path, "rb");
        if (!file) {
            fprintf(stderr, "Couldn't open '%s': %s\n",
                    keymap_path, strerror(errno));
            throw std::runtime_error("ERROR");  // TODO fix message
        }
        keymap = xkb_keymap_new_from_file(ctx, file,
                                          XKB_KEYMAP_FORMAT_TEXT_V1,
                                          XKB_KEYMAP_COMPILE_NO_FLAGS);
        fclose(file);
    }
    else {
        // const char *rules = NULL;
        // const char *model = "pc105";
        // const char *layout = "us";
        // const char *variant = "altgr-intl";
        // const char *options = "grp:alt_shift_toggle";

        // struct xkb_rule_names rmlvo = {
        //     .rules = (rules == NULL || rules[0] == '\0') ? NULL : rules,
        //     .model = (model == NULL || model[0] == '\0') ? NULL : model,
        //     .layout = (layout == NULL || layout[0] == '\0') ? NULL : layout,
        //     .variant = (variant == NULL || variant[0] == '\0') ? NULL : variant,
        //     .options = (options == NULL || options[0] == '\0') ? NULL : options
        // };

        // if (!rules && !model && !layout && !variant && !options)
        //     keymap = xkb_keymap_new_from_names(ctx, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
        // else

        struct xkb_rule_names rmlvo = {
            .rules = "evdev",
            .model = "pc105",
            .layout = "us",
            .variant = "qwerty",
            .options = "grp:alt_shift_toggle"
        };
        keymap = xkb_keymap_new_from_names(ctx, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
         //xkb_keymap_new_from_names(ctx, &rmlvo, XKB_KEYMAP_COMPILE_NO_FLAGS);

        if (!keymap) {
            fprintf(stderr,
                    "Failed to compile RMLVO: '%s', '%s', '%s', '%s', '%s'\n",
                    rmlvo.rules, rmlvo.model, rmlvo.layout, rmlvo.variant, rmlvo.options);
            throw std::runtime_error("ERROR");  // TODO fix message
        }
    }


    // Probing keyboards
    std::unique_ptr<input::Keyboard> keyboard;
    fs::path dri_path = "/dev/input";
    std::string card_path;
    for (const auto & entry : fs::directory_iterator(dri_path)) {
        card_path = entry.path();
        if (card_path.find(std::string(dri_path / "event")) == 0) {
            keyboard = std::make_unique<input::Keyboard>(card_path);
            if (keyboard->initializeAndProbe(keymap, nullptr)) {
                break;  // Success
            }
            keyboard = nullptr;
        }
    }
    if (keyboard == nullptr) {
        throw std::runtime_error("Can't find a keyboard.");
    }
    std::cout << "Using keyboard device: " << card_path << std::endl;
    return keyboard;
}

void Platform::run(std::shared_ptr<graphics::DisplayContentsFactory> contentsFactory) {
    
    auto card = probeCard(enableOpenGL);

    xkb_context* ctx = xkb_context_new(XKB_CONTEXT_NO_DEFAULT_INCLUDES);
    if (!ctx) {
        throw errcode_exception(-1, "Couldn't create xkb context");
    }

    const char *includes[64];
    size_t num_includes = 0;
    bool verbose = false;
    if (verbose) {
        xkb_context_set_log_level(ctx, XKB_LOG_LEVEL_DEBUG);
        xkb_context_set_log_verbosity(ctx, 10);
    }

    #define DEFAULT_INCLUDE_PATH_PLACEHOLDER "__defaults__"
    if (num_includes == 0)
        includes[num_includes++] = DEFAULT_INCLUDE_PATH_PLACEHOLDER;

    for (size_t i = 0; i < num_includes; i++) {
        const char *include = includes[i];
        if (strcmp(include, DEFAULT_INCLUDE_PATH_PLACEHOLDER) == 0)
            xkb_context_include_path_append_default(ctx);
        else
            xkb_context_include_path_append(ctx, include);
    }

    // Probing keyboards
    auto kbd = probeKeyboard(ctx);

    card->setDisplayContentsFactory(contentsFactory);
    card->runDrawingLoop();

    printf("exiting\n");
}

Platform::~Platform() {
    
}

}