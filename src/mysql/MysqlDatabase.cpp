#include "MysqlDatabase.h"
#include "System.h"

namespace {
struct MySQLLoader {
    MySQLLoader() {
        mysql_library_init(0, nullptr, nullptr);
    }
    ~MySQLLoader() {
        mysql_library_end();
    }
}_;
}

MysqlDatabase::MysqlDatabase()
: port_(0)
, max_connnection_count_(0)
{
}

MysqlDatabase::~MysqlDatabase()
{
    for (; !connections_.empty(); connections_.pop_front()) {
        DeleteConnection(connections_.front().first);
    }
}

bool MysqlDatabase::Connect(const char *host, unsigned int port,
                            const char *user, const char *passwd, const char *db,
                            unsigned int connInit, unsigned int connMax)
{
    host_ = host;
    port_ = port;
    user_ = user;
    passwd_ = passwd;
    db_ = db;
    max_connnection_count_ = connMax;

    for (unsigned int count = 0; count < connInit; ++count) {
        MysqlConnection *conn = NewConnection();
        if (conn != nullptr) {
            connections_.push_front({conn, GET_UNIX_TIME});
        } else {
            break;
        }
    }

    return connections_.size() >= connInit;
}

MysqlConnection *MysqlDatabase::GetConnection()
{
    MysqlConnection *conn = nullptr;
    do {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!connections_.empty()) {
            conn = connections_.front().first;
            connections_.pop_front();
        }
    } while (0);
    if (conn == nullptr) {
        conn = NewConnection();
    }
    return conn;
}

void MysqlDatabase::PutConnection(MysqlConnection *conn)
{
    std::lock_guard<std::mutex> lock(mutex_);
    connections_.push_front({conn, GET_UNIX_TIME});
}

void MysqlDatabase::CheckConnections()
{
    while (true) {
        MysqlConnection *conn = nullptr;
        bool isOverflow = false;
        do {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!connections_.empty()) {
                const time_t timeOut = (isOverflow =
                    connections_.size() > max_connnection_count_) ? 60 : 360;
                if (connections_.back().second + timeOut < GET_UNIX_TIME) {
                    conn = connections_.back().first;
                    connections_.pop_back();
                }
            }
        } while (0);
        if (conn != nullptr) {
            if (isOverflow) {
                DeleteConnection(conn);
            } else {
                if (conn->Ping() != 0) {
                    DeleteConnection(conn);
                } else {
                    PutConnection(conn);
                }
            }
        } else {
            break;
        }
    }
}

MysqlConnection *MysqlDatabase::NewConnection() const
{
    MysqlConnection *conn = new MysqlConnection;
    if (!conn->Connect(host_.c_str(), port_,
                       user_.c_str(), passwd_.c_str(), db_.c_str())) {
        DeleteConnection(conn);
        conn = nullptr;
    }
    return conn;
}

void MysqlDatabase::DeleteConnection(MysqlConnection *conn)
{
    delete conn;
}

std::unique_ptr<MysqlConnection, MysqlDatabase::ConnDeleter> MysqlDatabase::GetConnAutoPtr()
{
    return {GetConnection(), {this}};
}
