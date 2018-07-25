#pragma once

#include <stdexcept>
#include <sstream>
#include <unordered_map>
#include <vector>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "table_interface.h"
#include "Base.h"

namespace FieldHelper {
    template<typename T> std::string ToString(T entity) {
        std::ostringstream stream;
        stream << entity;
        return stream.str();
    }
    template<> inline std::string ToString(int8 entity) {
        std::ostringstream stream;
        stream << (int)entity;
        return stream.str();
    }
    template<> inline std::string ToString(uint8 entity) {
        std::ostringstream stream;
        stream << (int)entity;
        return stream.str();
    }
    template<> inline std::string ToString(std::string entity) {
        return entity;
    }

    template<typename T> void FromString(T &entity, const char *value) {
        entity = static_cast<T>(atoi(value));
    }
#define FROM_STRING(TYPE,GETTER) \
    template<> inline void FromString(TYPE &entity, const char *value) { \
        entity = GETTER; \
    }
    FROM_STRING(char, value[0])
    FROM_STRING(bool, atoi(value)!=0)
    FROM_STRING(float, strtof(value,nullptr))
    FROM_STRING(double, strtod(value,nullptr))
    FROM_STRING(uint32, strtoul(value,nullptr,0))
    FROM_STRING(int64, strtoll(value,nullptr,0))
    FROM_STRING(uint64, strtoull(value,nullptr,0))
    FROM_STRING(std::string, std::string(value))
#undef FROM_STRING
}

namespace StreamHelper {
    template<typename T> void ResizeWithPeek(T &entity, std::streamsize number, const std::istream &stream) {
        if (stream.rdbuf()->in_avail() >= number && number >= 0) { entity.clear(); entity.resize(number); }
        else { throw std::out_of_range("resize parameter is too large."); }
    }
    template<typename T> void ReserveWithPeek(T &entity, std::streamsize number, const std::istream &stream) {
        if (stream.rdbuf()->in_avail() >= number && number >= 0) { entity.clear(); entity.reserve(number); }
        else { throw std::out_of_range("reserve parameter is too large."); }
    }

    template<typename T> void FromStream(T &value, std::istream &stream) {
        stream.read((char *)&value, sizeof(value));
    }
    template<typename T> void ToStream(T value, std::ostream &stream) {
        stream.write((const char *)&value, sizeof(value));
    }
    template<> inline void FromStream(std::string &value, std::istream &stream) {
        uint32 size = 0;
        FromStream(size, stream);
        ResizeWithPeek(value, size, stream);
        stream.read((char *)value.data(), size);
    }
    template<> inline void ToStream(std::string value, std::ostream &stream) {
        uint32 size = (uint32)value.size();
        ToStream(size, stream);
        stream.write(value.data(), size);
    }
    inline void FromStream(std::vector<bool>::reference value, std::istream &stream) {
        bool bvalue = false;
        FromStream(bvalue, stream);
        value = bvalue;
    }

    template<typename T> void EnumFromStream(T &entity, std::istream &stream) {
        int32 value = 0;
        FromStream(value, stream);
        entity = static_cast<T>(value);
    }
    template<typename T> void EnumToStream(T entity, std::ostream &stream) {
        int32 value = entity;
        ToStream(value, stream);
    }

    template<typename T> void SequenceFromStream(std::vector<T> &entity, std::istream &stream) {
        uint32 number = 0;
        FromStream(number, stream);
        ResizeWithPeek(entity, number, stream);
        for (uint32 index = 0; index < number; ++index) {
            FromStream(entity[index], stream);
        }
    }
    template<typename T> void SequenceToStream(const std::vector<T> &entity, std::ostream &stream) {
        uint32 number = (uint32)entity.size();
        ToStream(number, stream);
        for (uint32 index = 0; index < number; ++index) {
            ToStream(entity[index], stream);
        }
    }

    template<typename T> void BlockSequenceFromStream(std::vector<T> &entity, std::istream &stream) {
        uint32 number = 0;
        FromStream(number, stream);
        ResizeWithPeek(entity, number, stream);
        for (uint32 index = 0; index < number; ++index) {
            LoadFromStream(entity[index], stream);
        }
    }
    template<typename T> void BlockSequenceToStream(const std::vector<T> &entity, std::ostream &stream) {
        uint32 number = (uint32)entity.size();
        ToStream(number, stream);
        for (uint32 index = 0; index < number; ++index) {
            SaveToStream(entity[index], stream);
        }
    }

    template<typename K, typename T> void AssociativeFromStream(std::unordered_map<K, T> &entity, std::istream &stream) {
        uint32 number = 0;
        FromStream(number, stream);
        ReserveWithPeek(entity, number, stream);
        for (uint32 index = 0; index < number; ++index) {
            std::pair<K, T> pair;
            FromStream(pair.first, stream);
            FromStream(pair.second, stream);
            entity.insert(std::move(pair));
        }
    }
    template<typename K, typename T> void AssociativeToStream(const std::unordered_map<K, T> &entity, std::ostream &stream) {
        uint32 number = (uint32)entity.size();
        ToStream(number, stream);
        for (auto &pair : entity) {
            ToStream(pair.first, stream);
            ToStream(pair.second, stream);
        }
    }

    template<typename K, typename T> void BlockAssociativeFromStream(std::unordered_map<K, T> &entity, std::istream &stream) {
        uint32 number = 0;
        FromStream(number, stream);
        ReserveWithPeek(entity, number, stream);
        for (uint32 index = 0; index < number; ++index) {
            std::pair<K, T> pair;
            FromStream(pair.first, stream);
            LoadFromStream(pair.second, stream);
            entity.insert(std::move(pair));
        }
    }
    template<typename K, typename T> void BlockAssociativeToStream(const std::unordered_map<K, T> &entity, std::ostream &stream) {
        uint32 number = (uint32)entity.size();
        ToStream(number, stream);
        for (auto &pair : entity) {
            ToStream(pair.first, stream);
            SaveToStream(pair.second, stream);
        }
    }
}

class JsonHelper {
public:
    JsonHelper(rapidjson::Document::AllocatorType &allocator) : allocator_(allocator) {}
    static void SetJsonObjectValue(rapidjson::Value &value) { value.SetObject(); }

    template<typename T> static void FromJson(T &entity, const rapidjson::Value &parent, const char *name) {
        if (parent.IsObject()) {
            auto itr = parent.FindMember(name);
            if (itr != parent.MemberEnd()) {
                FromJson(entity, itr->value);
            }
        }
    }
    template<typename T> void ToJson(T entity, rapidjson::Value &parent, const char *name) {
        rapidjson::Value value;
        ToJson(entity, value);
        parent.AddMember(rapidjson::StringRef(name), value, allocator_);
    }

    template<typename T> static void BlockFromJson(T &entity, const rapidjson::Value &parent, const char *name) {
        if (parent.IsObject()) {
            auto itr = parent.FindMember(name);
            if (itr != parent.MemberEnd()) {
                BlockFromJson(entity, itr->value);
            }
        }
    }
    template<typename T> void BlockToJson(const T &entity, rapidjson::Value &parent, const char *name) {
        rapidjson::Value value;
        BlockToJson(entity, value);
        parent.AddMember(rapidjson::StringRef(name), value, allocator_);
    }

    template<typename T> static void SequenceFromJson(std::vector<T> &entity, const rapidjson::Value &parent, const char *name) {
        if (parent.IsObject()) {
            auto itr = parent.FindMember(name);
            if (itr != parent.MemberEnd()) {
                SequenceFromJson(entity, itr->value);
            }
        }
    }
    template<typename T> void SequenceToJson(const std::vector<T> &entity, rapidjson::Value &parent, const char *name) {
        rapidjson::Value value;
        SequenceToJson(entity, value);
        parent.AddMember(rapidjson::StringRef(name), value, allocator_);
    }

    template<typename T> static void BlockSequenceFromJson(std::vector<T> &entity, const rapidjson::Value &parent, const char *name) {
        if (parent.IsObject()) {
            auto itr = parent.FindMember(name);
            if (itr != parent.MemberEnd()) {
                BlockSequenceFromJson(entity, itr->value);
            }
        }
    }
    template<typename T> void BlockSequenceToJson(const std::vector<T> &entity, rapidjson::Value &parent, const char *name) {
        rapidjson::Value value;
        BlockSequenceToJson(entity, value);
        parent.AddMember(rapidjson::StringRef(name), value, allocator_);
    }

    template<typename K, typename T> static void AssociativeFromJson(std::unordered_map<K, T> &entity, const rapidjson::Value &parent, const char *name) {
        if (parent.IsObject()) {
            auto itr = parent.FindMember(name);
            if (itr != parent.MemberEnd()) {
                AssociativeFromJson(entity, itr->value);
            }
        }
    }
    template<typename K, typename T> void AssociativeToJson(const std::unordered_map<K, T> &entity, rapidjson::Value &parent, const char *name) {
        rapidjson::Value value;
        AssociativeToJson(entity, value);
        parent.AddMember(rapidjson::StringRef(name), value, allocator_);
    }

    template<typename K, typename T> static void BlockAssociativeFromJson(std::unordered_map<K, T> &entity, const rapidjson::Value &parent, const char *name) {
        if (parent.IsObject()) {
            auto itr = parent.FindMember(name);
            if (itr != parent.MemberEnd()) {
                BlockAssociativeFromJson(entity, itr->value);
            }
        }
    }
    template<typename K, typename T> void BlockAssociativeToJson(const std::unordered_map<K, T> &entity, rapidjson::Value &parent, const char *name) {
        rapidjson::Value value;
        BlockAssociativeToJson(entity, value);
        parent.AddMember(rapidjson::StringRef(name), value, allocator_);
    }

    template<typename T> static void TableOptionalsFromJsonText(T &table, const char *text) {
        rapidjson::Document document;
        document.Parse(text);
        TableOptionalsFromJson(table, document);
    }
    template<typename T> static std::string TableOptionalsToJsonText(const T &table) {
        rapidjson::Document document;
        JsonHelper(document.GetAllocator()).TableOptionalsToJson(table, document);
        return JsonToText(document);
    }

    template<typename T> static void BlockFromJsonText(T &entity, const char *text) {
        rapidjson::Document document;
        document.Parse(text);
        BlockFromJson(entity, document);
    }
    template<typename T> static std::string BlockToJsonText(const T &entity) {
        rapidjson::Document document;
        JsonHelper(document.GetAllocator()).BlockToJson(entity, document);
        return JsonToText(document);
    }

    template<typename T> static void SequenceFromJsonText(std::vector<T> &entity, const char *text) {
        rapidjson::Document document;
        document.Parse(text);
        SequenceFromJson(entity, document);
    }
    template<typename T> static std::string SequenceToJsonText(const std::vector<T> &entity) {
        rapidjson::Document document;
        JsonHelper(document.GetAllocator()).SequenceToJson(entity, document);
        return JsonToText(document);
    }

    template<typename T> static void BlockSequenceFromJsonText(std::vector<T> &entity, const char *text) {
        rapidjson::Document document;
        document.Parse(text);
        BlockSequenceFromJson(entity, document);
    }
    template<typename T> static std::string BlockSequenceToJsonText(const std::vector<T> &entity) {
        rapidjson::Document document;
        JsonHelper(document.GetAllocator()).BlockSequenceToJson(entity, document);
        return JsonToText(document);
    }

    template<typename K, typename T> static void AssociativeFromJsonText(std::unordered_map<K, T> &entity, const char *text) {
        rapidjson::Document document;
        document.Parse(text);
        AssociativeFromJson(entity, document);
    }
    template<typename K, typename T> static std::string AssociativeToJsonText(const std::unordered_map<K, T> &entity) {
        rapidjson::Document document;
        JsonHelper(document.GetAllocator()).AssociativeToJson(entity, document);
        return JsonToText(document);
    }

    template<typename K, typename T> static void BlockAssociativeFromJsonText(std::unordered_map<K, T> &entity, const char *text) {
        rapidjson::Document document;
        document.Parse(text);
        BlockAssociativeFromJson(entity, document);
    }
    template<typename K, typename T> static std::string BlockAssociativeToJsonText(const std::unordered_map<K, T> &entity) {
        rapidjson::Document document;
        JsonHelper(document.GetAllocator()).BlockAssociativeToJson(entity, document);
        return JsonToText(document);
    }

private:
    rapidjson::Document::AllocatorType &allocator_;

    static std::string JsonToText(const rapidjson::Value &value) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        value.Accept(writer);
        return std::string(buffer.GetString(), buffer.GetSize());
    }

    template<typename T> static void TableOptionalsFromJson(T &entity, const rapidjson::Value &value);
    template<typename T> void TableOptionalsToJson(const T &entity, rapidjson::Value &value);

    template<typename T> static void BlockFromJson(T &entity, const rapidjson::Value &value);
    template<typename T> void BlockToJson(const T &entity, rapidjson::Value &value);

    template<typename T> static void FromJson(T &entity, const rapidjson::Value &value);
    template<typename T> void ToJson(T entity, rapidjson::Value &value);
    static void FromJson(std::vector<bool>::reference entity, const rapidjson::Value &value) {
        if (value.IsBool()) entity = value.GetBool();
    }

    template<typename T> static void EnumFromJson(T &entity, const rapidjson::Value &value) {
        if (value.IsInt()) entity = static_cast<T>(value.GetInt());
    }
    template<typename T> void EnumToJson(T entity, rapidjson::Value &value) {
        value.SetInt(entity);
    }

    template<typename T> static void SequenceFromJson(std::vector<T> &entity, const rapidjson::Value &value) {
        if (value.IsArray()) {
            size_t number = value.Size();
            entity.clear(), entity.resize(number);
            for (size_t index = 0; index < number; ++index) {
                FromJson(entity[index], value[index]);
            }
        }
    }
    template<typename T> void SequenceToJson(const std::vector<T> &entity, rapidjson::Value &value) {
        size_t number = entity.size();
        value.SetArray().Reserve(number, allocator_);
        for (size_t index = 0; index < number; ++index) {
            rapidjson::Value gvalue;
            ToJson(entity[index], gvalue);
            value.PushBack(gvalue, allocator_);
        }
    }

    template<typename T> static void BlockSequenceFromJson(std::vector<T> &entity, const rapidjson::Value &value) {
        if (value.IsArray()) {
            size_t number = value.Size();
            entity.clear(), entity.resize(number);
            for (size_t index = 0; index < number; ++index) {
                BlockFromJson(entity[index], value[index]);
            }
        }
    }
    template<typename T> void BlockSequenceToJson(const std::vector<T> &entity, rapidjson::Value &value) {
        size_t number = entity.size();
        value.SetArray().Reserve(number, allocator_);
        for (size_t index = 0; index < number; ++index) {
            rapidjson::Value gvalue;
            BlockToJson(entity[index], gvalue);
            value.PushBack(gvalue, allocator_);
        }
    }

    template<typename K, typename T> static void AssociativeFromJson(std::unordered_map<K, T> &entity, const rapidjson::Value &value) {
        if (value.IsObject()) {
            size_t number = value.MemberCount();
            entity.clear(), entity.reserve(number);
            for (auto itr = value.MemberBegin(); itr != value.MemberEnd(); ++itr) {
                std::pair<K, T> pair;
                FromJson(pair.first, itr->name);
                FromJson(pair.second, itr->value);
                entity.insert(std::move(pair));
            }
        }
    }
    template<typename K, typename T> void AssociativeToJson(const std::unordered_map<K, T> &entity, rapidjson::Value &value) {
        size_t number = entity.size();
        value.SetObject().MemberReserve(number, allocator_);
        for (auto &pair : entity) {
            rapidjson::Value name, gvalue;
            ToJson(pair.first, name);
            ToJson(pair.second, gvalue);
            value.AddMember(name, gvalue, allocator_);
        }
    }

    template<typename K, typename T> static void BlockAssociativeFromJson(std::unordered_map<K, T> &entity, const rapidjson::Value &value) {
        if (value.IsObject()) {
            size_t number = value.MemberCount();
            entity.clear(), entity.reserve(number);
            for (auto itr = value.MemberBegin(); itr != value.MemberEnd(); ++itr) {
                std::pair<K, T> pair;
                FromJson(pair.first, itr->name);
                BlockFromJson(pair.second, itr->value);
                entity.insert(std::move(pair));
            }
        }
    }
    template<typename K, typename T> void BlockAssociativeToJson(const std::unordered_map<K, T> &entity, rapidjson::Value &value) {
        size_t number = entity.size();
        value.SetObject().MemberReserve(number, allocator_);
        for (auto &pair : entity) {
            rapidjson::Value name, gvalue;
            ToJson(pair.first, name);
            BlockToJson(pair.second, gvalue);
            value.AddMember(name, gvalue, allocator_);
        }
    }
};

#define JSON_CONVERTER(TYPE,NAME) \
    template<> inline void JsonHelper::FromJson(TYPE &entity, const rapidjson::Value &value) { \
        if (value.Is##NAME()) entity = static_cast<TYPE>(value.Get##NAME()); \
    } \
    template<> inline void JsonHelper::ToJson(TYPE entity, rapidjson::Value &value) { \
        value.Set##NAME(entity); \
    }
    JSON_CONVERTER(char, Int)
    JSON_CONVERTER(bool, Bool)
    JSON_CONVERTER(float, Double)
    JSON_CONVERTER(double, Double)
    JSON_CONVERTER(int8, Int)
    JSON_CONVERTER(uint8, Int)
    JSON_CONVERTER(int16, Int)
    JSON_CONVERTER(uint16, Int)
    JSON_CONVERTER(int32, Int)
    JSON_CONVERTER(uint32, Uint)
    JSON_CONVERTER(int64, Int64)
    JSON_CONVERTER(uint64, Uint64)
#undef JSON_CONVERTER
template<> inline void JsonHelper::FromJson(std::string &entity, const rapidjson::Value &value) {
    if (value.IsString()) entity.assign(value.GetString(), value.GetStringLength());
}
template<> inline void JsonHelper::ToJson(std::string entity, rapidjson::Value &value) {
    value.SetString(entity.data(), entity.size(), allocator_);
}
