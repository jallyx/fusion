#pragma once

#include <istream>
#include <ostream>

template<typename T> void LoadFromStream(T &entity, std::istream &stream);
template<typename T> void SaveToStream(const T &entity, std::ostream &stream);

template<typename T> void LoadFromText(T &entity, const char *text);
template<typename T> std::string SaveToText(const T &entity);

template<typename T> const char *GetTableName();
template<typename T> const char *GetTableKeyName();
template<typename T, typename KEY> KEY GetTableKeyValue(const T &entity);
template<typename T, typename KEY> void SetTableKeyValue(T &entity, KEY key);
template<typename T> const char *GetTableFieldNameByIndex(size_t index);
template<typename T> ssize_t GetTableFieldIndexByName(const char *name);
template<typename T> size_t GetTableFieldNumber();
template<typename T> std::string GetTableFieldValue(const T &entity, size_t index);
template<typename T> void SetTableFieldValue(T &entity, size_t index, const char *value);
template<typename T> void InitTableValue(T &entity);
