#include <vector>
#include <string>
#include <cstdint>

std::pair<int, int> createSubprocessWithPty(uint16_t rows, uint16_t cols, const char* prog, const std::vector<std::string>& args = {}, const char* TERM = "xterm-256color");
std::pair<pid_t,int> waitpid(pid_t pid, int options);
