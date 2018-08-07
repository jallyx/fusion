#include "MysqlConnection.h"
#include <stdarg.h>
#include "Logger.h"

#define FormatSql(sql,fmt) \
    do { \
        va_list ap; \
        va_start(ap, fmt); \
        sql.resize(vsnprintf(nullptr, 0, fmt, ap) + 1); \
        va_end(ap); \
        va_start(ap, fmt); \
        sql.resize(vsnprintf((char*)sql.data(), sql.size(), fmt, ap)); \
        va_end(ap); \
    } while (0)

MysqlConnection::MysqlConnection()
: mysql_(nullptr)
{
}

MysqlConnection::~MysqlConnection()
{
    Close();
}

bool MysqlConnection::Connect(const char *host, unsigned int port,
                              const char *user, const char *passwd, const char *db)
{
    NLOG("Connecting to `%s`, database `%s`...", host, db);

    MYSQL *mysql = mysql_init(nullptr);
    if (mysql == nullptr) {
        ELOG("MysqlConnection: Init connection failed.");
        return false;
    }

    const bool optval = true;
    if (mysql_options(mysql, MYSQL_OPT_RECONNECT, &optval) != 0) {
        WLOG("MysqlConnection: Could not set reconnect mode.");
    }

    const unsigned long flags = CLIENT_FOUND_ROWS | CLIENT_MULTI_STATEMENTS;
    mysql_ = mysql_real_connect(mysql, host, user, passwd, db, port, nullptr, flags);
    if (mysql_ == nullptr) {
        ELOG("MysqlConnection: Connect failed due to: `%s`.", mysql_error(mysql));
        mysql_close(mysql);
        return false;
    }

    if (mysql_set_character_set(mysql, "utf8mb4") != 0) {
        WLOG("MysqlConnection: Could not set utf8mb4 character set.");
    }

    return true;
}

void MysqlConnection::Close()
{
    if (mysql_ != nullptr) {
        NLOG("Close from `%s`, database `%s`.", mysql_->host, mysql_->db);
        mysql_close(mysql_);
        mysql_ = nullptr;
    }
}

std::string MysqlConnection::EscapeString(const std::string &str) const
{
    return EscapeString(str.data(), str.size());
}

std::string MysqlConnection::EscapeString(const char *data, size_t size) const
{
    std::string str(size * 2 + 1, '\0');
    str.resize(mysql_ != nullptr ?
        mysql_real_escape_string(mysql_, (char*)str.data(), data, size) :
        mysql_escape_string((char*)str.data(), data, size));
    return str;
}

MysqlResult MysqlConnection::FastQuery(const char *sql) const
{
    return MysqlResult(CallSql(sql) ? mysql_use_result(mysql_) : nullptr);
}

MysqlResult MysqlConnection::FastQueryFormat(const char *fmt, ...) const
{
    std::string sql;
    FormatSql(sql, fmt);
    return FastQuery(sql.c_str());
}

MysqlResult MysqlConnection::Query(const char *sql) const
{
    return MysqlResult(CallSql(sql) ? mysql_store_result(mysql_) : nullptr);
}

MysqlResult MysqlConnection::QueryFormat(const char *fmt, ...) const
{
    std::string sql;
    FormatSql(sql, fmt);
    return Query(sql.c_str());
}

std::pair<my_ulonglong, bool> MysqlConnection::Execute(const char *sql) const
{
    static const std::pair<my_ulonglong, bool> failure(0, false);
    return CallSql(sql) ? std::make_pair(mysql_affected_rows(mysql_), true) : failure;
}

std::pair<my_ulonglong, bool> MysqlConnection::ExecuteFormat(const char *fmt, ...) const
{
    std::string sql;
    FormatSql(sql, fmt);
    return Execute(sql.c_str());
}

int MysqlConnection::Ping() const
{
    return mysql_ping(mysql_);
}

my_ulonglong MysqlConnection::GetInsertID() const
{
    return mysql_insert_id(mysql_);
}

unsigned int MysqlConnection::GetLastErrno() const
{
    return mysql_errno(mysql_);
}

const char *MysqlConnection::GetLastError() const
{
    return mysql_error(mysql_);
}

bool MysqlConnection::CallSql(const char *sql) const
{
    if (mysql_ == nullptr) {
        WLOG("MySQL connection is invalid.");
        return false;
    }

    int result = mysql_query(mysql_, sql);
    if (result != 0) {
        printf("%s\n", sql);
        ELOG("MySQL query failed due to: `%s`", mysql_error(mysql_));
        return false;
    }

    return true;
}
