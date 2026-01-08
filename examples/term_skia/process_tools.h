#include <vector>
#include <string>
#include <cstdint>
#include <functional>

#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

class TermSubprocess {
private:
    int pid, fd;
    uint16_t rows, cols;

    static std::pair<int, int> createSubprocessWithPty(uint16_t rows, uint16_t cols, const char* prog, const std::vector<std::string>& args = {}, const char* TERM = "xterm-256color");
    static std::pair<pid_t,int> waitpid(pid_t pid, int options);
    static void setPtySize(int fd, uint16_t rows, uint16_t cols);
public:
    TermSubprocess(uint16_t rows, uint16_t cols, const std::string& prog, const std::vector<std::string>& args = { })
        : rows(rows), cols(cols) {

        std::tie(pid, fd) = createSubprocessWithPty(rows, cols, prog.c_str(), args);
    }

    bool isExited() const {
        std::pair<pid_t, int> rst;
        rst = waitpid(this->pid, WNOHANG);
        return rst.first == pid;
    }

    void readInputAndProcess(std::function<void(const std::string&)> cb) {
        std::string input_cache;

        // 1. Reading

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        timeval timeout = { 0, 0 };

        while (select(fd + 1, &readfds, NULL, NULL, &timeout) > 0) {
            char buf[cols]; // one line. The maximum possible value for this buffer is 4096
            auto size = read(fd, buf, sizeof(buf));

            if (size > 0) {
                input_cache.append(buf, size);
                if (input_cache.size() > cols * rows * 4) break;
            }
        }

        // 2. Processing

        int single_print_size = cols * 2;
        while (input_cache.size() > single_print_size) {
            auto s1 = input_cache.substr(0, single_print_size);
            //input_write(s1.data(), s1.size());
            cb(s1);
            input_cache = input_cache.substr(s1.size());
        }
        
        if (input_cache.size() > 0) {
            //input_write(input_cache.data(), input_cache.size());
            cb(input_cache);
            input_cache = "";
        }

    }

    void write(const std::string& str) {
        ::write(fd, str.data(), str.length());
    }

    int getFd() const {
        return fd;
    }

};
