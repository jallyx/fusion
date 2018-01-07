#pragma once

#include <string>
#include <unordered_map>

class KeyFile
{
public:
    bool ParseFile(const char *file);

    const char *GetValue(const char *group, const char *key) const;

    const char *GetString(const char *group, const char *key, const char *tips) const;
    int GetInteger(const char *group, const char *key, int tips) const;

private:
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> data_;
};
