#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QString>

class DatabaseManager {
public:
    static DatabaseManager& instance();

    void initialize(const QString& dbPath = "");
    QSqlDatabase& database();
    bool isInitialized() const;

    // 禁止拷贝和移动
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

private:
    DatabaseManager() = default;
    ~DatabaseManager();

    QSqlDatabase m_db;
    bool m_initialized = false;
};

#endif // DATABASEMANAGER_H
