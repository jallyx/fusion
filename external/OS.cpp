#include "OS.h"
#include <thread>

#if defined(_WIN32)
    #include <io.h>
    #include <direct.h>
    #pragma comment(lib, "Shlwapi.lib")
#else
    #include <sys/stat.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <fcntl.h>
    #include <libgen.h>
    #include <limits.h>
    #include <unistd.h>
#endif

void OS::SleepS(long s)
{
    std::this_thread::sleep_for(std::chrono::seconds(s));
}

void OS::SleepMS(long ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void OS::CreateFilePath(const char *fullpath)
{
    std::string filepath(fullpath);
    size_t offset = 0;
    while (true) {
        size_t pos = filepath.find(os::sep, offset);
        if (pos != std::string::npos) {
#if defined(_WIN32)
            if (pos != 0 && filepath[pos - 1] != ':' && filepath[pos - 1] != '\\') {
#else
            if (pos != 0 && filepath[pos - 1] != '/') {
#endif
                filepath[pos] = '\0';
#if defined(_WIN32)
                _mkdir(filepath.c_str());
#else
                mkdir(filepath.c_str(), 0777);
#endif
                filepath[pos] = os::sep;
            }
            offset = pos + 1;
        } else {
            break;
        }
    }
}

void OS::CreateUnixFilePath(const char *fullpath)
{
    std::string filepath(fullpath);
    size_t offset = 0;
    while (true) {
        size_t pos = filepath.find('/', offset);
        if (pos != std::string::npos) {
            if (pos != 0 && filepath[pos - 1] != '/') {
                filepath[pos] = '\0';
#if defined(_WIN32)
                _mkdir(filepath.c_str());
#else
                mkdir(filepath.c_str(), 0777);
#endif
                filepath[pos] = '/';
            }
            offset = pos + 1;
        } else {
            break;
        }
    }
}

std::string OS::GetProgramDirectory()
{
#if defined(_WIN32)
    char buffer[MAX_PATH];
    if (GetModuleFileNameA(nullptr, buffer, sizeof(buffer)) != 0 &&
        PathRemoveFileSpecA(buffer)) {
        return buffer;
    }
#else
    char buffer[PATH_MAX];
    ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (length != -1) {
        buffer[length] = '\0';
        return dirname(buffer);
    }
#endif
    return "";
}

std::string OS::GetProgramName()
{
#if defined(_WIN32)
    char buffer[MAX_PATH];
    if (GetModuleFileNameA(nullptr, buffer, sizeof(buffer)) != 0) {
        return PathFindFileNameA(buffer);
    }
#else
    char buffer[PATH_MAX];
    ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (length != -1) {
        buffer[length] = '\0';
        return basename(buffer);
    }
#endif
    return "";
}

bool OS::is_file_exist(const char *filepath)
{
#if defined(_WIN32)
    return _access(filepath, 00) == 0;
#else
    return access(filepath, F_OK) == 0;
#endif
}

bool OS::non_blocking(SOCKET sockfd)
{
#if defined(_WIN32)
    u_long opt = 1;
    return ioctlsocket(sockfd, FIONBIO, &opt) == 0;
#else
    const int flags = fcntl(sockfd, F_GETFL, 0);
    return flags != -1 && fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

bool OS::reuse_address(SOCKET sockfd)
{
#if defined(_WIN32)
    const BOOL opt = 1;
    return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) == 0;
#else
    const int opt = 1;
    return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0;
#endif
}

bool OS::no_delay(SOCKET sockfd)
{
#if defined(_WIN32)
    const BOOL opt = 1;
    return setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const char *)&opt, sizeof(opt)) == 0;
#else
    const int opt = 1;
    return setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) == 0;
#endif
}
