#define LOGIT_FILE_LOGGER_PATH "."
#include <LogIt.hpp>

#include <fstream>
#include <string>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace {
#if defined(_WIN32)
std::string format_windows_error(DWORD code) {
    LPSTR buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD length = ::FormatMessageA(flags,
                                          nullptr,
                                          code,
                                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                          reinterpret_cast<LPSTR>(&buffer),
                                          0,
                                          nullptr);
    std::string result;
    if (length != 0 && buffer != nullptr) {
        result.assign(buffer, length);
        ::LocalFree(buffer);
        while (!result.empty() && (result.back() == '\r' || result.back() == '\n')) {
            result.pop_back();
        }
    }
    return result;
}
#endif
} // namespace

int main() {
    LOGIT_ADD_FILE_LOGGER_DEFAULT();

    const std::string message_token = "Failed to open configuration";
    std::string code_token;
    std::string expected_desc;

#if defined(_WIN32)
    ::SetLastError(ERROR_FILE_NOT_FOUND);
    const DWORD captured_code = ::GetLastError();
    LOGIT_SYSERR_ERROR(message_token.c_str());
    code_token = "GetLastError=" + std::to_string(static_cast<unsigned long>(captured_code));
    expected_desc = format_windows_error(captured_code);
#else
    errno = 0;
    int fd = ::open("/this/path/should/not/exist/logit.test", O_RDONLY);
    if (fd >= 0) {
        ::close(fd);
        return 1;
    }
    const int captured_errno = errno;
    expected_desc = std::strerror(captured_errno);
    errno = captured_errno;
    LOGIT_SYSERR_ERROR(message_token.c_str());
    code_token = "errno=" + std::to_string(captured_errno);
#endif

    LOGIT_WAIT();

    std::ifstream in(LOGIT_GET_LAST_FILE_PATH(0));
    if (!in.is_open()) {
        LOGIT_SHUTDOWN();
        return 1;
    }

    bool found = false;
    std::string line;
    while (std::getline(in, line)) {
        if (line.find(message_token) != std::string::npos &&
            line.find(code_token) != std::string::npos &&
            (expected_desc.empty() || line.find(expected_desc) != std::string::npos)) {
            found = true;
            break;
        }
    }

    LOGIT_SHUTDOWN();
    return found ? 0 : 1;
}
