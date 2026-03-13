#ifndef USERDAO_H
#define USERDAO_H

#include <QSqlError>
#include <QSqlQuery>
#include "user.h"

/// @brief 用户数据访问对象类
/// @details 负责所有与用户相关的数据库操作，包括表的创建、数据的增删改查等。
///          该类为静态类，禁止实例化，所有方法均为静态方法。
///
/// @note 所有方法都依赖于 DatabaseManager 提供的数据库连接，使用前请确保
///       DatabaseManager 已正确初始化。
class UserDao
{
public:
    /// @brief 获取数据库连接
    /// @return QSqlDatabase 的引用，指向 DatabaseManager 管理的数据库连接
    /// @warning 如果 DatabaseManager 未初始化，调用此方法将导致程序终止
    static QSqlDatabase& getDatabase();

    // 表管理--------------------------------------------------------------------------
    /// @brief 启用外键约束
    /// @details 执行 "PRAGMA foreign_keys = ON" 命令，启用 SQLite 的外键约束支持。
    ///          外键约束对于保证数据完整性非常重要，应在所有表创建前调用。
    ///
    /// @note 如果启用失败，程序将终止
    /// @see createAllTables()
    static void enableForeignKeys();

    /// @brief 创建所有数据表
    /// @details 按依赖顺序创建所有表：
    ///          1. User表 - 用户表（独立）
    ///          2. Category表 - 分类表（独立）
    ///          3. Expense表 - 支出记录表（依赖User和Category）
    ///
    /// @note 如果任一表创建失败，程序将终止
    /// @see createTableUser(), createTableCategory(), createTableExpense()
    static void createAllTables();

    /// @brief 创建用户表
    /// @details 创建 User 表，包含字段：
    ///          - user_id: 用户ID，主键，自增
    ///          - user_name: 用户名，唯一，非空
    ///          - user_pwd: 用户密码，非空
    static void createTableUser();

    /// @brief 创建分类表
    /// @details 创建 Category 表，包含字段：
    ///          - category_id: 分类ID，主键，自增
    ///          - category_name: 分类名称，非空
    static void createTableCategory();

    /// @brief 创建支出记录表
    /// @details 创建 Expense 表，包含字段：
    ///          - expense_id: 记录ID，主键，自增
    ///          - user_id: 用户ID，外键关联User表
    ///          - category_id: 分类ID，外键关联Category表
    ///          - date: 日期，默认为当前日期
    ///          - amount: 金额，必须大于0
    ///          - description: 描述
    ///
    /// @note 表包含外键约束，级联删除
    static void createTableExpense();

    /// @brief 创建所有索引
    /// @details 为优化查询性能创建以下索引：
    ///          - User表：user_name索引
    ///          - Category表：category_name索引
    ///          - Expense表：
    ///            * user_id索引
    ///            * category_id索引
    ///            * date索引
    ///            * (user_id, date)复合索引
    ///            * (user_id, category_id, date)复合索引
    static void createIndexes();

    /// @brief 插入测试数据
    /// @details 向所有表中插入预设的测试数据，包括：
    ///          - 一个测试用户（admin/123456）
    ///          - 6个测试分类（餐饮、交通、购物等）
    ///          - 20条测试支出记录（2025年11月-12月数据）
    ///
    /// @note 使用事务保证数据一致性，如果已有数据则跳过插入
    static void insertTestData();

    // 输入数据检查--------------------------------------------------------------------------
    /// @brief 检查用户名是否已存在
    /// @param username 要检查的用户名
    /// @return true 表示用户名已存在，false 表示不存在或查询失败
    static bool usernameExists(const QString& username);

    /// @brief 验证用户名密码是否匹配
    /// @param username 用户名
    /// @param password 密码（明文）
    /// @return true 表示用户名和密码匹配，false 表示不匹配或查询失败
    static bool passwordCorrect(const QString& username, const QString& password);

    // 插入数据--------------------------------------------------------------------------
    /// @brief 插入新用户
    /// @param user User对象指针，包含用户名和密码信息
    /// @return true 表示插入成功，false 表示插入失败
    /// @warning 此方法目前有bug，密码绑定使用了用户名，需要修复
    static bool insertUser(const User* user);

    // 获取ID--------------------------------------------------------------------------
    /// @brief 根据用户名获取用户ID
    /// @param name 用户名
    /// @return 成功返回用户ID，失败或用户不存在返回-1
    static qint32 getIdByName(const QString& name);

private:
    /// @brief 私有构造函数，禁止实例化
    UserDao() = delete;
};

#endif // USERDAO_H
