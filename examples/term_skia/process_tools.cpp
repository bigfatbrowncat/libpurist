#include "process_tools.h"

#include <pty.h>
#include <sys/wait.h>
#include <stdexcept>
#include <cstring.h>

std::pair<int, int> createSubprocessWithPty(uint16_t rows, uint16_t cols, const char* prog, const std::vector<std::string>& args, const char* TERM)
{
    int fd;
    struct winsize win = { rows, cols, 0, 0 };
    auto pid = forkpty(&fd, NULL, NULL, &win);
    if (pid < 0) throw std::runtime_error("forkpty failed");
    //else
    if (!pid) {
        setenv("TERM", TERM, 1);
        char ** argv = new char *[args.size() + 2];
        argv[0] = strdup(prog);
        for (int i = 1; i <= args.size(); i++) {
            argv[i] = strdup(args[i - 1].c_str());
        }
        argv[args.size() + 1] = NULL;
        if (execvp(prog, argv) < 0) exit(-1);
    }
    //else 
    return { pid, fd };
}

std::pair<pid_t,int> waitpid(pid_t pid, int options)
{
    int status;
    auto done_pid = waitpid(pid, &status, options);
    return {done_pid, status};
}
