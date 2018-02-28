#pragma once

#include <mutex>
#include <queue>
#include "MysqlConnection.h"

class MysqlDatabase : public noncopyable
{
public:
    MysqlDatabase();
    ~MysqlDatabase();

    bool Connect(const char *host, unsigned int port,
                 const char *user, const char *passwd, const char *db,
                 unsigned int connInit = 1, unsigned int connMax = 5);

    MysqlConnection *GetConnection();
    void PutConnection(MysqlConnection *conn);

private:
    MysqlConnection *NewConnection() const;
    static void DeleteConnection(MysqlConnection *conn);

    std::string host_, user_, passwd_, db_;
    unsigned int port_;

    std::mutex mutex_;
    std::queue<MysqlConnection*> connections_;
    unsigned int max_connnection_count_;
};
