#include "MysqlDatabase.h"

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
    for (; !connections_.empty(); connections_.pop()) {
        DeleteConnection(connections_.top());
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
            connections_.push(conn);
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
            conn = connections_.top();
            connections_.pop();
        }
    } while (0);
    if (conn == nullptr) {
        conn = NewConnection();
    }
    return conn;
}

void MysqlDatabase::PutConnection(MysqlConnection *conn)
{
    do {
        std::lock_guard<std::mutex> lock(mutex_);
        if (connections_.size() < max_connnection_count_) {
            connections_.push(conn);
            conn = nullptr;
        }
    } while (0);
    if (conn != nullptr) {
        DeleteConnection(conn);
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
