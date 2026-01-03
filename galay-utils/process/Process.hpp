#ifndef GALAY_UTILS_PROCESS_HPP
#define GALAY_UTILS_PROCESS_HPP

#include "../common/Defn.hpp"
#include <string>
#include <vector>
#include <optional>
#include <array>
#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#include <tlhelp32.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#endif

namespace galay::utils {

#if defined(_WIN32)
using ProcessId = DWORD;
#else
using ProcessId = pid_t;
#endif

struct ExitStatus {
    int code;
    bool signaled;
    int signal;

    bool success() const { return !signaled && code == 0; }
};

class Process {
public:
    static ProcessId currentId() {
#if defined(_WIN32)
        return GetCurrentProcessId();
#else
        return getpid();
#endif
    }

    static ProcessId parentId() {
#if defined(_WIN32)
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return 0;
        }

        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe);
        DWORD currentPid = GetCurrentProcessId();
        DWORD parentPid = 0;

        if (Process32First(hSnapshot, &pe)) {
            do {
                if (pe.th32ProcessID == currentPid) {
                    parentPid = pe.th32ParentProcessID;
                    break;
                }
            } while (Process32Next(hSnapshot, &pe));
        }

        CloseHandle(hSnapshot);
        return parentPid;
#else
        return getppid();
#endif
    }

    static std::optional<ExitStatus> wait(ProcessId pid, int options = 0) {
#if defined(_WIN32)
        HANDLE hProcess = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (!hProcess) {
            return std::nullopt;
        }

        DWORD waitResult = WaitForSingleObject(hProcess, (options & 1) ? 0 : INFINITE);
        if (waitResult == WAIT_TIMEOUT) {
            CloseHandle(hProcess);
            return std::nullopt;
        }

        DWORD exitCode;
        GetExitCodeProcess(hProcess, &exitCode);
        CloseHandle(hProcess);

        return ExitStatus{static_cast<int>(exitCode), false, 0};
#else
        int status;
        pid_t result = waitpid(pid, &status, options);

        if (result <= 0) {
            return std::nullopt;
        }

        ExitStatus exitStatus{0, false, 0};

        if (WIFEXITED(status)) {
            exitStatus.code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            exitStatus.signaled = true;
            exitStatus.signal = WTERMSIG(status);
        }

        return exitStatus;
#endif
    }

    static ProcessId spawn(const std::string& path, const std::vector<std::string>& args) {
#if defined(_WIN32)
        std::string cmdLine = "\"" + path + "\"";
        for (const auto& arg : args) {
            cmdLine += " \"" + arg + "\"";
        }

        STARTUPINFOA si = {sizeof(si)};
        PROCESS_INFORMATION pi;

        if (!CreateProcessA(nullptr, const_cast<char*>(cmdLine.c_str()),
                            nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
            return static_cast<ProcessId>(-1);
        }

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return pi.dwProcessId;
#else
        pid_t pid = fork();

        if (pid < 0) {
            return static_cast<ProcessId>(-1);
        }

        if (pid == 0) {
            std::vector<char*> argv;
            argv.push_back(const_cast<char*>(path.c_str()));
            for (const auto& arg : args) {
                argv.push_back(const_cast<char*>(arg.c_str()));
            }
            argv.push_back(nullptr);

            execv(path.c_str(), argv.data());
            _exit(127);
        }

        return pid;
#endif
    }

    static ExitStatus execute(const std::string& command) {
#if defined(_WIN32)
        int result = std::system(command.c_str());
        return ExitStatus{result, false, 0};
#else
        int result = std::system(command.c_str());

        ExitStatus status{0, false, 0};
        if (WIFEXITED(result)) {
            status.code = WEXITSTATUS(result);
        } else if (WIFSIGNALED(result)) {
            status.signaled = true;
            status.signal = WTERMSIG(result);
        }

        return status;
#endif
    }

    static std::pair<ExitStatus, std::string> executeWithOutput(const std::string& command) {
        std::string output;
        ExitStatus status{0, false, 0};

#if defined(_WIN32)
        FILE* pipe = _popen(command.c_str(), "r");
#else
        FILE* pipe = popen(command.c_str(), "r");
#endif

        if (!pipe) {
            status.code = -1;
            return {status, output};
        }

        std::array<char, 128> buffer;
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            output += buffer.data();
        }

#if defined(_WIN32)
        int result = _pclose(pipe);
        status.code = result;
#else
        int result = pclose(pipe);
        if (WIFEXITED(result)) {
            status.code = WEXITSTATUS(result);
        } else if (WIFSIGNALED(result)) {
            status.signaled = true;
            status.signal = WTERMSIG(result);
        }
#endif

        return {status, output};
    }

    static bool kill(ProcessId pid, int signal) {
#if defined(_WIN32)
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (!hProcess) {
            return false;
        }

        BOOL result = TerminateProcess(hProcess, static_cast<UINT>(signal));
        CloseHandle(hProcess);
        return result != 0;
#else
        return ::kill(pid, signal) == 0;
#endif
    }

    static bool isRunning(ProcessId pid) {
#if defined(_WIN32)
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProcess) {
            return false;
        }

        DWORD exitCode;
        BOOL result = GetExitCodeProcess(hProcess, &exitCode);
        CloseHandle(hProcess);

        return result && exitCode == STILL_ACTIVE;
#else
        return ::kill(pid, 0) == 0;
#endif
    }

    static bool daemonize() {
#if defined(_WIN32)
        return false;
#else
        pid_t pid = fork();
        if (pid < 0) {
            return false;
        }
        if (pid > 0) {
            _exit(0);
        }

        if (setsid() < 0) {
            return false;
        }

        pid = fork();
        if (pid < 0) {
            return false;
        }
        if (pid > 0) {
            _exit(0);
        }

        if (chdir("/") < 0) {
            return false;
        }

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        int fd = open("/dev/null", O_RDWR);
        if (fd >= 0) {
            dup2(fd, STDIN_FILENO);
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            if (fd > STDERR_FILENO) {
                close(fd);
            }
        }

        return true;
#endif
    }
};

} // namespace galay::utils

#endif // GALAY_UTILS_PROCESS_HPP
