#pragma once

#include <string>
#include "MysqlResult.h"
#include "noncopyable.h"

class MysqlConnection : public noncopyable
{
public:
    MysqlConnection();
    ~MysqlConnection();

    bool Connect(const char *host, unsigned int port,
                 const char *user, const char *passwd, const char *db);
    void Close();

    std::string EscapeString(const std::string &str) const;
    std::string EscapeString(const char *data, size_t size) const;

    MysqlResult FastQuery(const char *sql) const;
    MysqlResult FastQueryFormat(const char *fmt, ...) const;

    MysqlResult Query(const char *sql) const;
    MysqlResult QueryFormat(const char *fmt, ...) const;

    std::pair<my_ulonglong, bool> Execute(const char *sql) const;
    std::pair<my_ulonglong, bool> ExecuteFormat(const char *fmt, ...) const;

    int Ping() const;

    my_ulonglong GetInsertID() const;
    unsigned int GetLastErrno() const;
    const char *GetLastError() const;

    operator bool() const { return mysql_ != nullptr; }
    bool operator!() const { return mysql_ == nullptr; }

private:
    bool CallSql(const char *sql) const;

    MYSQL *mysql_;
};
