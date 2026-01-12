#include "Keyboards.h"
#include "Keyboard.h"
#include <cerrno>
#include <purist/exceptions.h>

#include <vector>
#include <cstring>
#include <iostream>
#include <xkbcommon/xkbcommon.h>

namespace purist::input {

Keyboards::Keyboards() {

}

Keyboards::~Keyboards() {
    if (ctx) {
        xkb_context_unref(ctx);
    }
    if (keymap) {
        xkb_keymap_unref(keymap);
    }
    if (compose_table) {
        xkb_compose_table_unref(compose_table);
    }
}

void Keyboards::initialize() {
    bool no_default = true;

    if (no_default) {
        ctx = xkb_context_new(XKB_CONTEXT_NO_DEFAULT_INCLUDES);
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

        #define DEFAULT_INCLUDE_PATH_PLACEHOLDER "/usr/share/X11/xkb/"
        if (num_includes == 0) {
            includes[num_includes++] = DEFAULT_INCLUDE_PATH_PLACEHOLDER;
            includes[num_includes++] = "/usr/share/X11/locale/";
        }

        for (size_t i = 0; i < num_includes; i++) {
            const char *include = includes[i];
            if (strcmp(include, DEFAULT_INCLUDE_PATH_PLACEHOLDER) == 0)
                xkb_context_include_path_append_default(ctx);
            else
                xkb_context_include_path_append(ctx, include);
        }
    } else {
        ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        if (!ctx) {
            throw errcode_exception(-1, "Couldn't create xkb context");
        }
        xkb_context_include_path_append_default(ctx);
    }

    /////////////


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
            .rules = nullptr,
            .model = nullptr,//"pc105",
            .layout = "us,ru,il",
            .variant = nullptr,
            .options = "grp:alt_shift_toggle"
        };

        // keymap = xkb_keymap_new_from_names(ctx, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);

        keymap = xkb_keymap_new_from_names(ctx, &rmlvo, XKB_KEYMAP_COMPILE_NO_FLAGS);

        if (!keymap) {
            // fprintf(stderr,
            //         "Failed to compile RMLVO: '%s', '%s', '%s', '%s', '%s'\n",
            //         rmlvo.rules, rmlvo.model, rmlvo.layout, rmlvo.variant, rmlvo.options);
            throw std::runtime_error("ERROR");  // TODO fix message
        }
    }

    if (!keymap) {
        throw std::runtime_error("Couldn't create xkb keymap");
    }

    if (with_compose) {
        char* locale = setlocale(LC_CTYPE, NULL);
        compose_table =
//            xkb_compose_table_new_from_locale(ctx, locale,
//                                              XKB_COMPOSE_COMPILE_NO_FLAGS);

        compose_table =
            xkb_compose_table_new_from_file(ctx, fopen("/usr/share/X11/locale/en_US.UTF-8/Compose", "r"), "en_US.UTF-8",
                XKB_COMPOSE_FORMAT_TEXT_V1, XKB_COMPOSE_COMPILE_NO_FLAGS); 

        if (!compose_table) {
            throw std::runtime_error("Couldn't create compose from locale");
        }
    }
}

void Keyboards::updateHardwareConfiguration(std::shared_ptr<input::KeyboardHandler> keyboardHandler) {
    // Probing keyboards
    std::shared_ptr<input::Keyboard> keyboard;
    fs::path input_path = "/dev/input/by-path/";
    std::string suffix = "-event-kbd";
    std::string device_path;
    if (fs::is_directory(input_path)) {
        for (const auto & entry : fs::directory_iterator(input_path)) {
            device_path = entry.path();
            if (device_path.find(std::string(suffix)) == device_path.length() - suffix.size()) {
                // Looking for an existing keyboard
                bool exists = false;
                for (auto kbd : *this) {
                    if (kbd->getNode() == device_path) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    keyboard = std::make_unique<input::Keyboard>(device_path);
                    if (keyboard->initializeAndProbe(keymap, compose_table, keyboardHandler)) {
                        std::cout << "Adding found keyboard: " << device_path << std::endl;
                        this->push_back(keyboard);
                        //break;  // Success
                    }
                }
                //keyboard = nullptr;
            }
        }
    }
}

std::vector<pollfd> Keyboards::getFds() {
    nfds_t nfds = this->size();

    std::vector<pollfd> fds(nfds);

    auto fds_iter = fds.begin();
    for (auto& kbd : *this) {
        fds_iter->fd = kbd->getFd();
        fds_iter->events = POLLIN;
        fds_iter->revents = 0;
        
        fds_iter++;
    }
    return fds;
}

void Keyboards::processFd(std::vector<pollfd>::iterator fds_iter)
{
    std::shared_ptr<Keyboard> found_kbd = nullptr;
    for (auto& kbd : *this) {
        if (kbd->getFd() == fds_iter->fd) {
            found_kbd = kbd;
            break;
        }
    }

    if (found_kbd == nullptr) {
        throw errcode_exception(-1, std::string("Can't find a keyboard with fd = ") + std::to_string(fds_iter->fd));
    }
    
    if (fds_iter->revents != 0) {
        if (!found_kbd->read_keyboard(with_compose)) {
            std::cerr << "Keyboard " << found_kbd->getNode() << " was disconnected" << std::endl;
            this->remove(found_kbd);
        }
    }
}

}
