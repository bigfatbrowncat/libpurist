#include "Keyboard.h"
#include <cstdio>
#include <purist/exceptions.h>

#include <linux/input.h>
#include <fcntl.h>

#include <unistd.h>
#include <climits>

#include <iostream>

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
}


bool Keyboard::initializeAndProbe(xkb_keymap *keymap, xkb_compose_table *compose_table) { 
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

//err_compose_state:
    //xkb_compose_state_unref(compose_state);
//err_state:
//    xkb_state_unref(state);
//err_fd:
//    close(fd);
//err_path:
    // free(path);
    //return ret;


    // std::cout << "Input device name: \"" << evdev->get_name() << "\"" << std::endl;

    // std::cout << "Input device ID: bus " << evdev->get_id_bustype()
    //           << " vendor "  << evdev->get_id_vendor()
    //           << " product " << evdev->get_id_product()
    //           << std::endl;
    
    // if (evdev->has_event_type<evdevw::event::Key>() &&
    //     evdev->has_event_code(evdevw::event::KeyCode::A)) {
    //         std::cerr << "This device looks like a keyboard" << std::endl;
    //     throw evdevw::Exception(-1);
    // } else if (evdev->has_event_type<evdevw::event::Relative>() &&
    //            evdev->has_event_code(evdevw::event::KeyCode::ButtonLeft)) {
    //         std::cerr << "This device looks like a mouse" << std::endl;
    //     throw evdevw::Exception(-1);
    // } else {
    //     throw evdevw::Exception(-1);
    // }

}


}