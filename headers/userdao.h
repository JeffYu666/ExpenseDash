#ifndef USERDAO_H
#define USERDAO_H

#include <QSqlError>
#include <QSqlQuery>
#include "user.h"

class UserDao {
public:
    // 表管理
    static void enableForeignKeys(); // 启用外键约束
    static void createAllTables();   // 创建所有表
    static void createIndexes();     // 创建所有索引
    static bool createTableUser();
    static bool createTableCategory();
    static bool createTableExpense();
    // 测试数据
    static void insertTestData();
    // 输入数据检查
    static bool usernameExists(const QString& username);
    static bool passwordCorrect(const QString& username, const QString& password);
    // 插入数据
    static bool insertUser(const User* user);
    // 获取ID
    static int getIdByName(const QString& name);

private:
    UserDao() = delete; // 禁止实例化
};

#endif // USERDAO_H
