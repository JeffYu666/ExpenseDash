#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QString>

/// @brief 数据库管理器类
/// @details 采用单例模式设计，负责管理唯一的数据库连接，提供数据库初始化、
///          状态查询和连接获取等功能。该类不可拷贝、不可移动。
class DatabaseManager
{
public:
    /// @brief 获取数据库管理器实例
    /// @return 数据库管理器实例的引用
    static DatabaseManager& getInstance();

    /// @brief 初始化数据库
    /// @details 在应用程序目录下创建数据目录并初始化SQLite数据库连接，
    ///          如果目录不存在会自动创建，数据库文件名为"database.db"。
    /// @param relativeDirName 数据目录名称，默认为"data"
    ///        （例如："data" -> 应用程序目录/data/）
    void initializeDatabase(const QString& relativeDirName = "data");

    /// @brief 返回数据库初始化状态
    /// @return 初始化成功返回true，否则返回false
    bool isInitialized();

    /// @brief 获取数据库连接对象
    /// @pre 数据库必须已初始化（isInitialized() 返回 true）
    /// @return QSqlDatabase 的引用
    /// @warning 如果数据库未初始化就调用此函数，程序会致命错误退出
    QSqlDatabase& getDatabase();

    // 禁用拷贝构造、拷贝赋值、移动构造和移动赋值
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    DatabaseManager(DatabaseManager&&) = delete;
    DatabaseManager& operator=(DatabaseManager&&) = delete;

private:
    /// @brief 私有构造函数，防止外部实例化
    DatabaseManager() = default;

    /// @brief 析构函数，负责关闭数据库连接
    ~DatabaseManager();

    QSqlDatabase m_db;         ///< 数据库连接对象
    bool m_initialized = false;///< 数据库初始化标志
};

#endif // DATABASEMANAGER_H
