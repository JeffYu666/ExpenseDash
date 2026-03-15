#include "userdao.h"
#include "databasemanager.h"

QSqlDatabase& UserDao::getDatabase()
{
    return DatabaseManager::getInstance().getDatabase();
}

// 表管理--------------------------------------------------------------------------
void UserDao::enableForeignKeys()
{
    QSqlDatabase& db = getDatabase();
    QSqlQuery query(db);
    if (!query.exec("PRAGMA foreign_keys = ON"))
        qFatal("无法启用外键约束！%s", qPrintable(query.lastError().text()));
    else
        qInfo().noquote() << "成功启用外键约束。";
}

void UserDao::createAllTables()
{
    UserDao::createTableUser();
    UserDao::createTableCategory();
    UserDao::createTableExpense();

    qInfo().noquote() << "所有表均创建成功。";
}

void UserDao::createTableUser()
{
    QSqlDatabase& db = getDatabase();
    QSqlQuery query(db);
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS User (
            user_id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_name VARCHAR(20) NOT NULL UNIQUE,
            user_hash VARCHAR(64) NOT NULL,
            user_salt VARCHAR(256) NOT NULL
        )
    )"))
    {
        qFatal("User表创建失败！%s", qPrintable(query.lastError().text()));
    }
}

void UserDao::createTableCategory()
{
    QSqlDatabase& db = getDatabase();
    QSqlQuery query(db);
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS Category (
            category_id INTEGER PRIMARY KEY AUTOINCREMENT,
            category_name VARCHAR(30) NOT NULL
        )
    )"))
    {
        qFatal("Category表创建失败！%s", qPrintable(query.lastError().text()));
    }
}

void UserDao::createTableExpense()
{
    QSqlDatabase& db = getDatabase();
    QSqlQuery query(db);
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS Expense (
            expense_id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER,
            category_id INTEGER,
            date DATE NOT NULL DEFAULT (date('now')),
            amount DECIMAL(10, 2) NOT NULL CHECK(amount > 0),
            description VARCHAR(50),
            FOREIGN KEY (user_id) REFERENCES User(user_id) ON DELETE CASCADE,
            FOREIGN KEY (category_id) REFERENCES Category(category_id) ON DELETE CASCADE
        )
    )"))
    {
        qFatal("Expense表创建失败！%s", qPrintable(query.lastError().text()));
    }
}

void UserDao::createIndexes()
{
    QSqlDatabase& db = getDatabase();
    QSqlQuery query(db);

    bool success = true;

    // 创建User表的索引
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_user_name ON User(user_name)"))
    {
        qWarning().noquote() << "User表索引创建失败！" << query.lastError().text();
        success = false;
    }

    // 创建Category表的索引
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_category_name ON Category(category_name)"))
    {
        qWarning().noquote() << "Category表索引创建失败！" << query.lastError().text();
        success = false;
    }

    // 创建Expense表的索引
    // 用户ID索引 - 用于按用户查询消费记录
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_expense_user_id ON Expense(user_id)"))
    {
        qWarning().noquote() << "Expense表user_id索引创建失败！" << query.lastError().text();
        success = false;
    }

    // 分类ID索引 - 用于按分类查询消费记录
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_expense_category_id ON Expense(category_id)"))
    {
        qWarning().noquote() << "Expense表category_id索引创建失败！" << query.lastError().text();
        success = false;
    }

    // 日期索引 - 用于按时间范围查询
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_expense_date ON Expense(date)"))
    {
        qWarning().noquote() << "Expense表date索引创建失败！" << query.lastError().text();
        success = false;
    }

    // 用户+日期复合索引 - 用于查询特定用户在特定时间段的消费
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_expense_user_date ON Expense(user_id, date)"))
    {
        qWarning().noquote() << "Expense表user_id+date索引创建失败！" << query.lastError().text();
        success = false;
    }

    // 用户+分类+日期复合索引 - 用于复杂的统计分析
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_expense_user_category_date ON Expense(user_id, "
                    "category_id, date)"))
    {
        qWarning().noquote() << "Expense表user_id+category_id+date索引创建失败！"
                             << query.lastError().text();
        success = false;
    }

    if (success)
        qInfo().noquote() << "所有索引创建成功";
}

// 插入测试数据--------------------------------------------------------------------------
namespace
{
    /// @brief 检查指定表是否为空
    /// @details 执行 SELECT COUNT(*) 查询来判断表中是否有数据。如果查询执行失败或表不存在，
    ///          函数会返回 false 并记录警告信息。此函数主要用于在初始化数据库时判断
    ///          是否需要插入测试数据。
    ///
    /// @param query  已连接的 QSqlQuery 对象，用于执行查询操作
    /// @param tableName 要检查的表名
    /// @return
    ///         - true  - 表存在且没有任何数据（记录数为0）
    ///         - false - 表不存在、查询失败或表中已有数据（记录数>0）
    ///
    /// @warning 函数不会检查表名是否合法，调用者需确保表名不包含 SQL 注入风险
    /// @note 如果查询执行失败，函数会自动记录警告信息，包括表名和错误详情
    ///
    /// @see insertTestData
    bool isTableEmpty(QSqlQuery& query, const QString& tableName)
    {
        QString sql = QString("SELECT COUNT(*) FROM %1").arg(tableName);
        if (!query.exec(sql))
        {
            qWarning().noquote() << "检查表" << tableName << "是否存在数据失败！" << Qt::endl
                                 << "    取消对表" << tableName << "的测试数据插入！" << Qt::endl
                                 << "    " << query.lastError().text();
            return false;
        }

        if (query.next())
            return query.value(0).toInt() == 0;

        return false;
    }
}// namespace

void UserDao::insertTestData()
{
    QSqlDatabase& db = getDatabase();
    QSqlQuery query(db);

    // 开始事务
    if (!db.transaction())
    {
        qWarning().noquote() << "插入测试数据的事务开始失败！" << db.lastError().text();
        return;
    }

    // 检查User表中是否已有数据，避免重复插入
    if (isTableEmpty(query, "User"))
    {
        User testUser(1, "admin", "123456");
        // 插入用户数据
        QStringList userQueries = {
            R"(INSERT INTO User (user_name, user_hash, user_salt)
               VALUES (:username, :userhash, :usersalt))",
        };

        for (const QString &sql : userQueries)
        {
            query.prepare(sql);
            query.bindValue(":username", testUser.getName());
            query.bindValue(":userhash", testUser.getHash());
            query.bindValue(":usersalt", testUser.getSalt());
            if (!query.exec())
            {
                qWarning().noquote() << "测试用户数据插入失败！" << query.lastError().text();
                db.rollback();
                return;
            }
        }
    }

    // 检查Category表中是否已有数据，避免重复插入
    if (isTableEmpty(query, "Category"))
    {
        // 准备预处理语句
        QString insertSql = "INSERT INTO Category (category_name) VALUES (:category_name)";
        query.prepare(insertSql);

        // 定义类别数据
        const QStringList categories = {
            "餐饮", "交通", "购物", "娱乐", "医疗", "教育"
        };

        for (const QString &category : categories)
        {
            query.bindValue(":category_name", category);

            if (!query.exec())
            {
                qWarning().noquote() << "测试类别数据插入失败！" << query.lastError().text();
                db.rollback();
                return;
            }
        }
    }

    // 检查Expense表中是否已有数据，避免重复插入
    if (isTableEmpty(query, "Expense"))
    {
        // 插入支出记录
        QString insertSql = "INSERT INTO Expense (user_id, category_id, date, amount, description) "
                             "VALUES (:user_id, :category_id, :date, :amount, :description)";

        query.prepare(insertSql);

        // 定义支出数据
        struct ExpenseRecord
        {
            qint32 categoryId;
            QString date;
            double amount;
            QString description;
        };

        const QVector<ExpenseRecord> expenses = {
            {1, "2025-12-01", 25.50, "午餐"},
            {2, "2025-12-10", 15.00, "地铁卡充值"},
            {3, "2025-12-12", 199.00, "买衣服"},
            {4, "2025-12-20", 80.00, "看电影"},
            {1, "2025-12-05", 38.00, "晚餐"},
            {5, "2025-12-15", 120.00, "感冒药"},
            {6, "2025-12-18", 299.00, "购买编程书籍"},
            {3, "2025-12-22", 45.50, "超市购物"},
            {2, "2025-12-25", 50.00, "出租车费"},
            {4, "2025-12-28", 150.00, "游乐园门票"},
            {1, "2025-12-02", 32.00, "早餐"},
            {2, "2025-12-05", 20.00, "公交车费"},
            {3, "2025-12-08", 89.90, "日用品采购"},
            {4, "2025-12-12", 65.00, "KTV唱歌"},
            {1, "2025-12-15", 45.80, "周末聚餐"},
            {5, "2025-11-18", 85.00, "维生素补充剂"},
            {6, "2025-11-20", 158.00, "在线课程"},
            {3, "2025-11-23", 210.00, "秋季新装"},
            {2, "2025-11-26", 35.00, "共享单车月卡"},
            {4, "2025-11-30", 120.00, "朋友聚会"}
        };

        const qint32 userId = 1;

        for (const auto &expense : expenses)
        {
            query.bindValue(":user_id", userId);
            query.bindValue(":category_id", expense.categoryId);
            query.bindValue(":date", expense.date);
            query.bindValue(":amount", expense.amount);
            query.bindValue(":description", expense.description);

            if (!query.exec())
            {
                qWarning().noquote() << "测试支出记录插入失败！" << query.lastError().text();
                db.rollback();
                return;
            }
        }
    }

    // 提交事务
    if (!db.commit())
    {
        qWarning().noquote() << "插入测试数据的事务提交失败！" << db.lastError().text();
        db.rollback();
        return;
    }

    qDebug() << "测试数据全部插入成功。";
}

// 输入数据检查--------------------------------------------------------------------------
bool UserDao::usernameExists(const QString &username)
{
    QSqlDatabase& db = getDatabase();
    QSqlQuery query(db);

    query.prepare("SELECT COUNT(*) FROM User WHERE user_name = :username");
    query.bindValue(":username", username);

    if (!query.exec())
    {
        qWarning().noquote() << "查询用户名是否存在失败！" << query.lastError().text();
        return true;// 查询用户名是否存在失败，默认已存在
    }

    if (query.next())
    {
        return query.value(0).toInt() > 0;
    }

    return false;
}

bool UserDao::passwordCorrect(const QString &username, const QString &password)
{
    QSqlDatabase& db = getDatabase();
    QSqlQuery query(db);

    query.prepare("SELECT user_hash, user_salt FROM User WHERE user_name = :username");
    query.bindValue(":username", username);

    if (!query.exec())
    {
        qWarning().noquote() << "读取哈希和盐失败！" << query.lastError().text();
        return false;
    }

    if (query.next())
    {
        QString hash = query.value("user_hash").toString();
        QString salt = query.value("user_salt").toString();
        return hash == User::hashPassword(password, salt);
    }

    return false;
}

// 插入数据--------------------------------------------------------------------------
bool UserDao::insertUser(const User *user)
{
    QSqlDatabase& db = getDatabase();
    QSqlQuery query(db);
    query.prepare("INSERT INTO User (user_name, user_hash, user_salt) VALUES (:username, :userhash, :usersalt)");
    query.bindValue(":username", user->getName());
    query.bindValue(":userhash", user->getHash());
    query.bindValue(":usersalt", user->getSalt());
    if (!query.exec())
    {
        qWarning().noquote() << "注册新用户时，插入User表失败！" << db.lastError().text();
        return false;
    }
    return true;
}

// 获取ID--------------------------------------------------------------------------
qint32 UserDao::getIdByName(const QString &name)
{
    QSqlDatabase& db = getDatabase();
    QSqlQuery query(db);
    query.prepare("SELECT user_id FROM User WHERE user_name = :username");
    query.bindValue(":username", name);

    if (!query.exec())
    {
        qWarning().noquote() << "通过用户名查询id失败！" << db.lastError().text();
        return -1;
    }
    if (query.next())
        return query.value(0).value<qint32>(); // 返回找到的user_id
    else
    {
        qWarning().noquote() << "通过用户名查询id时，未找到用户:" << name;
        return -1; // 用户不存在
    }
}
