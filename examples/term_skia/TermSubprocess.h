#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <functional>

class TermSubprocess {
private:
    int pid, fd;
    uint16_t rows, cols;

    static std::pair<int, int> createSubprocessWithPty(uint16_t rows, uint16_t cols, const char* prog, const std::vector<std::string>& args = {}, const char* TERM = "xterm-256color");
    static std::pair<pid_t,int> waitpid(pid_t pid, int options);
    static void setPtySize(int fd, uint16_t rows, uint16_t cols);
public:
    TermSubprocess(uint16_t rows, uint16_t cols, const std::string& prog, const std::vector<std::string>& args = { });

    bool isExited() const;
    void readInputAndProcess(std::function<bool(const std::string_view&)> cb);
    void write(const std::string& str);
    int getFd() const;
    
};
