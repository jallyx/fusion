#pragma once

#include <string>
#include "Macro.h"

class OS
{
public:
    static void SleepS(long s);
    static void SleepMS(long ms);

    static void CreateFilePath(const char *fullpath);
    static void CreateUnixFilePath(const char *fullpath);

    static std::string GetProgramDirectory();
    static std::string GetProgramName();

    static bool is_file_exist(const char *filepath);

    static bool non_blocking(SOCKET sockfd);
    static bool reuse_address(SOCKET sockfd);
    static bool no_delay(SOCKET sockfd);
};
