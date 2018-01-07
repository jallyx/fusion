#include "KeyFile.h"
#include <fstream>

bool KeyFile::ParseFile(const char *file)
{
    std::ifstream stream(file);
    if (!stream.is_open())
        return false;

    std::string group;
    std::string linedata;
    while (true) {
        if (stream.eof())
            break;
        std::getline(stream, linedata);
        if (stream.fail() || stream.bad())
            break;
        size_t key_begin_pos = linedata.find_first_not_of("\x20\t\r\n");
        size_t value_end_pos = linedata.find_last_not_of("\x20\t\r\n");
        if (value_end_pos == std::string::npos) continue;
        if (linedata.at(key_begin_pos) == '[' && linedata.at(value_end_pos) == ']') {
            group = linedata.substr(key_begin_pos + 1, value_end_pos - key_begin_pos - 1);
            continue;
        }
        if (linedata.at(key_begin_pos) == '#') continue;
        if (linedata.at(key_begin_pos) == '=') continue;
        size_t pos = linedata.find_first_of('=', key_begin_pos);
        if (pos == std::string::npos) continue;
        size_t key_end_pos = linedata.find_last_not_of("\x20\t", pos - 1);
        size_t value_begin_pos = linedata.find_first_not_of("\x20\t", pos + 1);
        if (value_begin_pos == std::string::npos) value_begin_pos = value_end_pos + 1;
        data_[group][linedata.substr(key_begin_pos, key_end_pos - key_begin_pos + 1)] =
            linedata.substr(value_begin_pos, value_end_pos - value_begin_pos + 1);
    }

    return true;
}

const char *KeyFile::GetValue(const char *group, const char *key) const
{
    auto group_iterator = data_.find(group);
    if (group_iterator == data_.end())
        return nullptr;

    auto &data = group_iterator->second;
    auto key_iterator = data.find(key);
    if (key_iterator == data.end())
        return nullptr;

    return key_iterator->second.c_str();
}

const char *KeyFile::GetString(const char *group, const char *key, const char *tips) const
{
    const char *value = GetValue(group, key);
    return value != nullptr ? value : tips;
}

int KeyFile::GetInteger(const char *group, const char *key, int tips) const
{
    const char *value = GetValue(group, key);
    return value != nullptr ? atoi(value) : tips;
}
