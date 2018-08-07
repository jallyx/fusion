#pragma once

#include <mysql.h>
#include <stdlib.h>
#include "Base.h"
#include "noncopyable.h"

class MysqlRow
{
public:
    MysqlRow(MYSQL_ROW rows, unsigned long *lens)
        : rows_(rows), lens_(lens)
    {}

    void Next() { ++rows_, ++lens_; }

    const char *GetValue() const { return *rows_; }
    const char *GetString() const { return *rows_ ? *rows_ : ""; }
    unsigned long GetLength() const { return *lens_; }

#define CONVERT_VALUE(TYPE,NAME,CONVERTER,DEFAULT) \
    TYPE Get##NAME() const { const char *s = *rows_; \
    return s ? static_cast<TYPE>(CONVERTER) : DEFAULT; }
    CONVERT_VALUE(int8, Int8, atoi(s), 0)
    CONVERT_VALUE(uint8, UInt8, atoi(s), 0)
    CONVERT_VALUE(int16, Int16, atoi(s), 0)
    CONVERT_VALUE(uint16, UInt16, atoi(s), 0)
    CONVERT_VALUE(int32, Int32, strtol(s,nullptr,0), 0)
    CONVERT_VALUE(uint32, UInt32, strtoul(s,nullptr,0), 0)
    CONVERT_VALUE(int64, Int64, strtoll(s,nullptr,0), 0)
    CONVERT_VALUE(uint64, UInt64, strtoull(s,nullptr,0), 0)
    CONVERT_VALUE(double, Double, strtod(s,nullptr), 0)
    CONVERT_VALUE(float, Float, strtof(s,nullptr), 0)
    CONVERT_VALUE(bool, Bool, atoi(s)!=0, false)
#undef CONVERT_VALUE

private:
    MYSQL_ROW rows_;
    unsigned long *lens_;
};

class MysqlResult : public noncopyable
{
public:
    MysqlResult(MYSQL_RES *res)
        : res_(res), rows_(nullptr), lens_(nullptr)
    {}
    MysqlResult(MysqlResult &&other) {
        res_ = other.res_; other.res_ = nullptr;
        rows_ = other.rows_; other.rows_ = nullptr;
        lens_ = other.lens_; other.lens_ = nullptr;
    }
    ~MysqlResult() {
        if (res_ != nullptr) {
            mysql_free_result(res_);
        }
    }

    bool NextRow() {
        rows_ = mysql_fetch_row(res_);
        lens_ = mysql_fetch_lengths(res_);
        return rows_ != nullptr && lens_ != nullptr;
    }

    MysqlRow Fetch() const {
        return MysqlRow(rows_, lens_);
    }

    my_ulonglong GetRowCount() const {
        return mysql_num_rows(res_);
    }
    unsigned int GetFieldCount() const {
        return mysql_num_fields(res_);
    }

    operator bool() const { return res_ != nullptr; }
    bool operator!() const { return res_ == nullptr; }

private:
    MYSQL_RES *res_;
    MYSQL_ROW rows_;
    unsigned long *lens_;
};
