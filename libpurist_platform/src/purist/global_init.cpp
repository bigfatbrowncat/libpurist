#include <cstdlib>
#include <iostream>

#include "global_init.h"

void fix_ld_library_path() {
    const char* LDLP = "LD_LIBRARY_PATH";

    std::string arch;
    #if defined(__x86_64__)
        arch = "x86_64";
    #elif defined(__i386__)
        arch = "x86";
    #elif defined(__aarch64__)
        arch = "aarch64";
    #elif defined(__arm__)
        arch = "arm";
    #endif

    std::string new_path = "/usr/lib/" + arch + "-linux-gnu";
    std::cout << "Appending " + new_path + " to " << LDLP << std::endl;

    char* e = getenv(LDLP);
    if (e == nullptr) {
        setenv(LDLP, new_path.c_str(), true);
    } else {
        setenv(LDLP, (std::string(e) + ":" + new_path).c_str(), true);
    }

}