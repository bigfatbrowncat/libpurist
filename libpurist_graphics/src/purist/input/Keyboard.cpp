#include "Keyboard.h"
#include <cstdio>
#include <purist/exceptions.h>

#include <linux/input.h>
#include <fcntl.h>

#include <stdexcept>
#include <unistd.h>
#include <climits>
#include <cstring>

#include <iostream>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>

namespace purist::input {

#define NLONGS(n) (((n) + LONG_BIT - 1) / LONG_BIT)

bool Keyboard::evdev_bit_is_set(const unsigned long *array, int bit)
{
    return array[bit / LONG_BIT] & (1LL << (bit % LONG_BIT));
}

bool Keyboard::is_keyboard(int fd)
{
    int i;
    unsigned long evbits[NLONGS(EV_CNT)] = { 0 };
    unsigned long keybits[NLONGS(KEY_CNT)] = { 0 };

    errno = 0;
    ioctl(fd, EVIOCGBIT(0, sizeof(evbits)), evbits);
    if (errno)
        return false;

    if (!evdev_bit_is_set(evbits, EV_KEY))
        return false;

    errno = 0;
    ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits);
    if (errno)
        return false;

    for (i = KEY_RESERVED; i <= KEY_MIN_INTERESTING; i++)
        if (evdev_bit_is_set(keybits, i))
            return true;

    return false;
}

Keyboard::Keyboard(const fs::path& node) : node(node), fd(-1) { 
}


Keyboard::~Keyboard() {
    if (fd >= 0) {
        close(fd);
    }
    if (state) {
        xkb_state_unref(state);
    }
    if (compose_state) {
        xkb_compose_state_unref(compose_state);
    }
}


bool Keyboard::initializeAndProbe(xkb_keymap *keymap, xkb_compose_table *compose_table, std::shared_ptr<KeyboardHandler> keyboardHandler) { 
    //int ret;
    //char *path;

    struct xkb_state *state;
    struct xkb_compose_state *compose_state = NULL;

    //struct keyboard *kbd;

    fd = open(node.c_str(), O_NONBLOCK | O_CLOEXEC | O_RDONLY);
    if (fd < 0) {
        throw errcode_exception(-errno, std::string("Can't open ") + std::string(node));
    }

    if (!is_keyboard(fd)) {
        /* Dummy "skip this device" value. */
        //throw errcode_exception(-ENOTSUP, std::string("Not a keyboard ") + node.string());
        //goto err_fd;
        return false;
    }

    this->keyboardHandler = keyboardHandler;

    // Cleaning up the keyboard input buffer
    int bytesread;
    char buf;
    while ((bytesread = read(fd, &buf, 1)) > 0) { }

    // Grabbing the keyboard
    ioctl(fd, EVIOCGRAB, (void*)1);

    state = xkb_state_new(keymap);
    if (!state) {
        //fprintf(stderr, "Couldn't create xkb state for %s\n", path);
        throw errcode_exception(-EFAULT, std::string("Couldn't create xkb state for ") + node.string());
        //goto err_fd;
    }

    if (compose_table) {
        compose_state = xkb_compose_state_new(compose_table,
                                              XKB_COMPOSE_STATE_NO_FLAGS);
        if (!compose_state) {
            //fprintf(stderr, "Couldn't create compose state for %s\n", path);
            xkb_state_unref(state);
            throw errcode_exception(-EFAULT, std::string("Couldn't create compose state for ") + node.string());
            //ret = -EFAULT;
            //goto err_state;
        }
    }

    this->state = state;
    this->compose_state = compose_state;
    return true;

}

static void
print_keycode(struct xkb_keymap *keymap, const char* prefix,
              xkb_keycode_t keycode, const char *suffix) {
    const char *keyname = xkb_keymap_key_get_name(keymap, keycode);
    if (keyname) {
        printf("%s%-4s%s", prefix, keyname, suffix);
    } else {
        printf("%s%-4d%s", prefix, keycode, suffix);
    }
}

void Keyboard::tools_print_keycode_state(const char *prefix,
                          struct xkb_state *state,
                          struct xkb_compose_state *compose_state,
                          xkb_keycode_t keycode,
                          enum xkb_consumed_mode consumed_mode)
{
    struct xkb_keymap *keymap;

    xkb_keysym_t sym;
    const xkb_keysym_t *syms;
    int nsyms;
    
    const int XKB_COMPOSE_MAX_STRING_SIZE = 256;
    const int XKB_KEYSYM_NAME_MAX_SIZE = 27;

    char s[std::max(XKB_COMPOSE_MAX_STRING_SIZE, XKB_KEYSYM_NAME_MAX_SIZE)];
    xkb_layout_index_t layout;
    enum xkb_compose_status status;

    keymap = xkb_state_get_keymap(state);

    nsyms = xkb_state_key_get_syms(state, keycode, &syms);

    if (nsyms <= 0)
        return;

    status = XKB_COMPOSE_NOTHING;
    if (compose_state)
        status = xkb_compose_state_get_status(compose_state);

    if (status == XKB_COMPOSE_COMPOSING || status == XKB_COMPOSE_CANCELLED)
        return;

    if (status == XKB_COMPOSE_COMPOSED) {
        sym = xkb_compose_state_get_one_sym(compose_state);
        syms = &sym;
        nsyms = 1;
    }
    else if (nsyms == 1) {
        sym = xkb_state_key_get_one_sym(state, keycode);
        syms = &sym;
    }

    if (prefix)
        printf("%s", prefix);

    print_keycode(keymap, "keycode [ ", keycode, " ] ");

#ifdef ENABLE_PRIVATE_APIS
    if (fields & PRINT_MODMAPS) {
        print_key_modmaps(keymap, keycode);
    }
#endif

    printf("keysyms [ ");
    for (int i = 0; i < nsyms; i++) {
        xkb_keysym_get_name(syms[i], s, sizeof(s));
        printf("%-*s ", XKB_KEYSYM_NAME_MAX_SIZE, s);
    }
    printf("] ");

    if (/*fields & PRINT_UNICODE*/ true) {
        if (status == XKB_COMPOSE_COMPOSED) {
            xkb_compose_state_get_utf8(compose_state, s, sizeof(s));
        } else {
            xkb_state_key_get_utf8(state, keycode, s, sizeof(s));
        }

        if (*s != 0 && keyboardHandler != nullptr) { 
            keyboardHandler->onCharacter(*this, s);
        }

        /* HACK: escape single control characters from C0 set using the
        * Unicode codepoint convention. Ideally we would like to escape
        * any non-printable character in the string.
        */
        if (!*s) {
            printf("unicode [   ] ");
        } else if (strlen(s) == 1 && (*s <= 0x1F || *s == 0x7F)) {
            printf("unicode [ U+%04hX ] ", *s);
        } else {
            printf("unicode [ %s ] ", s);
        }
    }

    layout = xkb_state_key_get_layout(state, keycode);
    if (/*fields & PRINT_LAYOUT*/true) {
        printf("layout [ %s (%d) ] ",
               xkb_keymap_layout_get_name(keymap, layout), layout);
    }

    printf("level [ %d ] ",
           xkb_state_key_get_level(state, keycode, layout));

    printf("mods [ ");
    for (xkb_mod_index_t mod = 0; mod < xkb_keymap_num_mods(keymap); mod++) {
        if (xkb_state_mod_index_is_active(state, mod,
                                          XKB_STATE_MODS_EFFECTIVE) <= 0)
            continue;
        if (xkb_state_mod_index_is_consumed2(state, keycode, mod,
                                             consumed_mode))
            printf("-%s ", xkb_keymap_mod_get_name(keymap, mod));
        else
            printf("%s ", xkb_keymap_mod_get_name(keymap, mod));
    }
    printf("] ");

    printf("leds [ ");
    for (xkb_led_index_t led = 0; led < xkb_keymap_num_leds(keymap); led++) {
        if (xkb_state_led_index_is_active(state, led) <= 0)
            continue;
        printf("%s ", xkb_keymap_led_get_name(keymap, led));
    }
    printf("] ");

    printf("\n");
}

void Keyboard::tools_print_state_changes(enum xkb_state_component changed)
{
    if (changed == 0)
        return;

    printf("changed [ ");
    if (changed & XKB_STATE_LAYOUT_EFFECTIVE)
        printf("effective-layout ");
    if (changed & XKB_STATE_LAYOUT_DEPRESSED)
        printf("depressed-layout ");
    if (changed & XKB_STATE_LAYOUT_LATCHED)
        printf("latched-layout ");
    if (changed & XKB_STATE_LAYOUT_LOCKED)
        printf("locked-layout ");
    if (changed & XKB_STATE_MODS_EFFECTIVE)
        printf("effective-mods ");
    if (changed & XKB_STATE_MODS_DEPRESSED)
        printf("depressed-mods ");
    if (changed & XKB_STATE_MODS_LATCHED)
        printf("latched-mods ");
    if (changed & XKB_STATE_MODS_LOCKED)
        printf("locked-mods ");
    if (changed & XKB_STATE_LEDS)
        printf("leds ");
    printf("]\n");
}

void Keyboard::process_event(uint16_t type, uint16_t code, int32_t value, bool with_compose)
{
   
    xkb_keycode_t keycode;
    struct xkb_keymap *keymap;
    enum xkb_state_component changed;
    enum xkb_compose_status status;

    if (type != EV_KEY)
        return;

    keycode = evdev_offset + code;
    keymap = xkb_state_get_keymap(this->state);

    // Processing the key in sequence

    if (value == KEY_STATE_REPEAT && !xkb_keymap_key_repeats(keymap, keycode))
        return;
    
    xkb_keysym_t keysym = xkb_state_key_get_one_sym(this->state, keycode);
    if (with_compose && value != KEY_STATE_RELEASE) {
        xkb_compose_state_feed(this->compose_state, keysym);
    }

    if (value != KEY_STATE_RELEASE) {
        tools_print_keycode_state(
            NULL, this->state, this->compose_state, keycode,
            this->consumed_mode//, print_fields
        );
    }

    if (with_compose) {
        status = xkb_compose_state_get_status(this->compose_state);
        if (status == XKB_COMPOSE_CANCELLED || status == XKB_COMPOSE_COMPOSED)
            xkb_compose_state_reset(this->compose_state);
    }

    if (value == KEY_STATE_RELEASE)
        changed = xkb_state_update_key(this->state, keycode, XKB_KEY_UP);
    else
        changed = xkb_state_update_key(this->state, keycode, XKB_KEY_DOWN);

    if (/*report_state_changes*/true)
        tools_print_state_changes(changed);

        
    Modifiers mods = modifiersFromKeymap(keymap, state, keycode, consumed_mode);
    Leds leds = ledsFromKeymap(keymap, state);

    if (value == KEY_STATE_REPEAT) {
        if (keyboardHandler != nullptr) {
            keyboardHandler->onKeyPress(*this, keysym, mods, leds, true);
        }
    } else if (value == KEY_STATE_PRESS) {
        if (keyboardHandler != nullptr) {
            keyboardHandler->onKeyPress(*this, keysym, mods, leds, false);
        }
    } else if (value == KEY_STATE_RELEASE) {
        if (keyboardHandler != nullptr) {
            keyboardHandler->onKeyRelease(*this, keysym, mods, leds);
        }
    } else {
        throw std::runtime_error("Impossible case");
    }

}

int Keyboard::read_keyboard(bool with_compose)
{
    ssize_t len;
    struct input_event evs[16];

    /* No fancy error checking here. */
    while ((len = read(this->fd, &evs, sizeof(evs))) > 0) {
        const size_t nevs = len / sizeof(struct input_event);
        for (size_t i = 0; i < nevs; i++)
            process_event(evs[i].type, evs[i].code, evs[i].value, with_compose);
    }

    if (len < 0 && errno != EWOULDBLOCK) {
        fprintf(stderr, "Couldn't read: %s\n", /*this->path,*/ strerror(errno));
        return 1;
    }

    return 0;
}



}