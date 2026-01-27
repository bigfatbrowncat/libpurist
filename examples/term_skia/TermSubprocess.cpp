#include "TermSubprocess.h"

// System headers
#include <pty.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <termios.h>
#include <fcntl.h> // For fcntl, F_GETFL, F_SETFL, O_NONBLOCK
#include <unistd.h>

// C headers
#include <cstring.h>

// C++ std headers
#include <stdexcept>
#include <iostream>

std::pair<int, int> TermSubprocess::createSubprocessWithPty(uint16_t rows, uint16_t cols, const char* prog, const std::vector<std::string>& args, const char* TERM)
{
    int fd;
    struct winsize win = { rows, cols, 0, 0 };
    auto pid = forkpty(&fd, NULL, NULL, &win);
    if (pid < 0) throw std::runtime_error("forkpty failed");

    if (!pid) {
        setenv("TERM", TERM, 1);
        setenv("COLORTERM", "truecolor", 1);
        char ** argv = new char *[args.size() + 2];
        argv[0] = strdup(prog);
        for (size_t i = 1; i <= args.size(); i++) {
            argv[i] = strdup(args[i - 1].c_str());
        }
        argv[args.size() + 1] = NULL;
        if (execvp(prog, argv) < 0) exit(-1);
    }

    // 1. Get current flags
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "fcntl(F_GETFL)" << std::endl;
        // Handle error
    }

    // 2. Add O_NONBLOCK flag
    flags |= O_NONBLOCK;

    // 3. Set new flags
    if (fcntl(fd, F_SETFL, flags) == -1) {
        std::cerr << "fcntl(F_SETFL)" << std::endl;
        // Handle error
    }

    return { pid, fd };
}

void TermSubprocess::setPtySize(int fd, uint16_t rows, uint16_t cols) {
    struct winsize size;
    size.ws_row = rows;
    size.ws_col = cols;
    size.ws_xpixel = 0;
    size.ws_ypixel = 0;

    if (ioctl(fd, TIOCSWINSZ, &size) < 0) {
        perror("ioctl TIOCSWINSZ failed");
    } else {
        printf("PTY size set to %dx%d\n", rows, cols);
    }
}

std::pair<pid_t,int> TermSubprocess::waitpid(pid_t pid, int options) {
    int status;
    auto done_pid = ::waitpid(pid, &status, options);
    return {done_pid, status};
}

TermSubprocess::TermSubprocess(uint16_t rows, uint16_t cols, const std::string& prog, const std::vector<std::string>& args)
    : rows(rows), cols(cols) {

    std::tie(pid, fd) = createSubprocessWithPty(rows, cols, prog.c_str(), args);
}

bool TermSubprocess::isExited() const {
    std::pair<pid_t, int> rst;
    rst = waitpid(this->pid, WNOHANG);
    return rst.first == pid;
}

void TermSubprocess::readInputAndProcess(std::function<bool(const std::string_view&)> cb) {
    
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    timeval timeout = { 0, 0 };

    int total_size = 0;
    while (select(fd + 1, &readfds, NULL, NULL, &timeout) > 0) {
        char buf[4096];//cols]; // one line. The maximum possible value for this buffer is 4096
        auto size = read(fd, buf, sizeof(buf));

        if (size > 0) {
            if (!cb(std::string_view(buf, size))) {
                break;
            }
            total_size += size;
            if (total_size > 1024*1024 /* 1 MB */) {
                // Enough for one frame
                break;
            }
        } else {
            break; // Can't read. Maybe the client app closed... Anyway, passing through
        }
    }

}

void TermSubprocess::write(const std::string& str) {
    ::write(fd, str.data(), str.length());
}

int TermSubprocess::getFd() const {
    return fd;
}
