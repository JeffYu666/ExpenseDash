#include "userdao.h"
#include "databasemanager.h"

// 表管理--------------------------------------------------------------------------
void UserDao::enableForeignKeys() { // 启用外键约束
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    if (!query.exec("PRAGMA foreign_keys = ON")) {
        qWarning() << "无法启用外键约束:" << query.lastError().text();
    } else {
        qDebug() << "成功启用外键约束。";
    }
}

void UserDao::createAllTables() { // 创建所有表
    if (UserDao::createTableUser() && UserDao::createTableCategory()
        && UserDao::createTableExpense()) {
        qDebug() << "所有表均创建成功。";
    }
}

bool UserDao::createTableUser() {
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS User (
            user_id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_name VARCHAR(20) NOT NULL UNIQUE,
            user_pwd VARCHAR(255) NOT NULL
        )
    )")) {
        qWarning() << "User表创建失败！" << query.lastError().text();
        return false;
    }
    return true;
}

bool UserDao::createTableCategory() {
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS Category (
            category_id INTEGER PRIMARY KEY AUTOINCREMENT,
            category_name VARCHAR(30) NOT NULL
        )
    )")) {
        qWarning() << "Category表创建失败！" << query.lastError().text();
        return false;
    }
    return true;
}

bool UserDao::createTableExpense() {
    QSqlDatabase db = DatabaseManager::instance().database();
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
    )")) {
        qWarning() << "Expense表创建失败！" << query.lastError().text();
        return false;
    }
    return true;
}

void UserDao::createIndexes() {
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    bool success = true;

    // User表的索引
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_user_name ON User(user_name)")) {
        qWarning() << "User表索引创建失败！" << query.lastError().text();
        success = false;
    }

    // Category表的索引
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_category_name ON Category(category_name)")) {
        qWarning() << "Category表索引创建失败！" << query.lastError().text();
        success = false;
    }

    // Expense表的索引
    // 用户ID索引 - 用于按用户查询消费记录
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_expense_user_id ON Expense(user_id)")) {
        qWarning() << "Expense表user_id索引创建失败！" << query.lastError().text();
        success = false;
    }

    // 分类ID索引 - 用于按分类查询消费记录
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_expense_category_id ON Expense(category_id)")) {
        qWarning() << "Expense表category_id索引创建失败！" << query.lastError().text();
        success = false;
    }

    // 日期索引 - 用于按时间范围查询
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_expense_date ON Expense(date)")) {
        qWarning() << "Expense表date索引创建失败！" << query.lastError().text();
        success = false;
    }

    // 用户+日期复合索引 - 用于查询特定用户在特定时间段的消费
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_expense_user_date ON Expense(user_id, date)")) {
        qWarning() << "Expense表user_id+date索引创建失败！" << query.lastError().text();
        success = false;
    }

    // 用户+分类+日期复合索引 - 用于复杂的统计分析
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_expense_user_category_date ON Expense(user_id, "
                    "category_id, date)")) {
        qWarning() << "Expense表user_id+category_id+date索引创建失败！" << query.lastError().text();
        success = false;
    }

    if (success) {
        qDebug() << "所有索引创建成功";
    }
}

// 测试数据--------------------------------------------------------------------------
void UserDao::insertTestData() {
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    // 开始事务
    if (!db.transaction()) {
        qWarning() << "事务开始失败！" << db.lastError().text();
        return;
    }

    // 检查是否已有数据，避免重复插入
    query.exec("SELECT COUNT(*) FROM User");
    if (query.next() && query.value(0).toInt() == 0) {
        // 插入用户数据
        QStringList userQueries = {
            "INSERT INTO User (user_name, user_pwd) VALUES ('admin', '123456')"};

        for (const QString &sql : userQueries) {
            if (!query.exec(sql)) {
                qWarning() << "用户数据插入失败！" << query.lastError().text();
                db.rollback();
                return;
            }
        }
    }

    // 检查是否已有数据，避免重复插入
    query.exec("SELECT COUNT(*) FROM Category");
    if (query.next() && query.value(0).toInt() == 0) {
        // 插入类别数据
        QStringList categoryQueries = {
            "INSERT INTO Category (category_name) VALUES ('餐饮')",
            "INSERT INTO Category (category_name) VALUES ('交通')",
            "INSERT INTO Category (category_name) VALUES ('购物')",
            "INSERT INTO Category (category_name) VALUES ('娱乐')",
            "INSERT INTO Category (category_name) VALUES ('医疗')",
            "INSERT INTO Category (category_name) VALUES ('教育')",
        };

        for (const QString &sql : categoryQueries) {
            if (!query.exec(sql)) {
                qWarning() << "类别数据插入失败！" << query.lastError().text();
                db.rollback();
                return;
            }
        }
    }

    // 检查是否已有数据，避免重复插入
    query.exec("SELECT COUNT(*) FROM Expense");
    if (query.next() && query.value(0).toInt() == 0) {
        // 插入支出记录
        QStringList expenseQueries
            = {"INSERT INTO Expense (user_id, category_id, date, amount, "
               "description) VALUES (1, 1, '2025-12-01', 25.50, '午餐')",
               "INSERT INTO Expense (user_id, category_id, date, amount, "
               "description) VALUES (1, 2, '2025-12-10', 15.00, '地铁卡充值')",
               "INSERT INTO Expense (user_id, category_id, date, amount, "
               "description) VALUES (1, 3, '2025-12-12', 199.00, '买衣服')",
               "INSERT INTO Expense (user_id, category_id, date, amount, "
               "description) VALUES (1, 4, '2025-12-20', 80.00, '看电影')",
               "INSERT INTO Expense (user_id, category_id, date, amount, "
               "description) VALUES (1, 1, '2025-12-05', 38.00, '晚餐')",
               "INSERT INTO Expense (user_id, category_id, date, amount, "
               "description) VALUES (1, 5, '2025-12-15', 120.00, '感冒药')",
               "INSERT INTO Expense (user_id, category_id, date, amount, "
               "description) VALUES (1, 6, '2025-12-18', 299.00, '购买编程书籍')",
               "INSERT INTO Expense (user_id, category_id, date, amount, "
               "description) VALUES (1, 3, '2025-12-22', 45.50, '超市购物')",
               "INSERT INTO Expense (user_id, category_id, date, amount, "
               "description) VALUES (1, 2, '2025-12-25', 50.00, '出租车费')",
               "INSERT INTO Expense (user_id, category_id, date, amount, "
               "description) VALUES (1, 4, '2025-12-28', 150.00, '游乐园门票')",
               "INSERT INTO Expense (user_id, category_id, date, amount, description) VALUES (1, "
               "1, '2025-12-02', 32.00, '早餐')",
               "INSERT INTO Expense (user_id, category_id, date, amount, description) VALUES (1, "
               "2, '2025-12-05', 20.00, '公交车费')",
               "INSERT INTO Expense (user_id, category_id, date, amount, description) VALUES (1, "
               "3, '2025-12-08', 89.90, '日用品采购')",
               "INSERT INTO Expense (user_id, category_id, date, amount, description) VALUES (1, "
               "4, '2025-12-12', 65.00, 'KTV唱歌')",
               "INSERT INTO Expense (user_id, category_id, date, amount, description) VALUES (1, "
               "1, '2025-12-15', 45.80, '周末聚餐')",
               "INSERT INTO Expense (user_id, category_id, date, amount, description) VALUES (1, "
               "5, '2025-11-18', 85.00, '维生素补充剂')",
               "INSERT INTO Expense (user_id, category_id, date, amount, description) VALUES (1, "
               "6, '2025-11-20', 158.00, '在线课程')",
               "INSERT INTO Expense (user_id, category_id, date, amount, description) VALUES (1, "
               "3, '2025-11-23', 210.00, '秋季新装')",
               "INSERT INTO Expense (user_id, category_id, date, amount, description) VALUES (1, "
               "2, '2025-11-26', 35.00, '共享单车月卡')",
               "INSERT INTO Expense (user_id, category_id, date, amount, description) VALUES (1, "
               "4, '2025-11-30', 120.00, '朋友聚会')"};

        for (const QString &sql : expenseQueries) {
            if (!query.exec(sql)) {
                qWarning() << "支出记录插入失败！" << query.lastError().text();
                db.rollback();
                return;
            }
        }
    }

    // 提交事务
    if (!db.commit()) {
        qWarning() << "事务提交失败！" << db.lastError().text();
        db.rollback();
        return;
    }

    qDebug() << "测试数据已就绪。";
}

// 输入数据检查--------------------------------------------------------------------------
bool UserDao::usernameExists(const QString &username) {
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    query.prepare("SELECT COUNT(*) FROM User WHERE user_name = :username");
    query.bindValue(":username", username);

    if (!query.exec()) {
        qWarning() << "查询用户名是否存在失败:" << query.lastError().text();
        return false;
    }

    if (query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}

bool UserDao::passwordCorrect(const QString &username, const QString &password) {
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);

    query.prepare("SELECT COUNT(*) FROM User WHERE user_name = :username AND user_pwd = :password");
    query.bindValue(":username", username);
    query.bindValue(":password", password);

    if (!query.exec()) {
        qWarning() << "查询密码是否匹配失败:" << query.lastError().text();
        return false;
    }

    if (query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}

// 插入数据--------------------------------------------------------------------------
bool UserDao::insertUser(const User *user) {
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("INSERT INTO User (user_name, user_pwd) VALUES (:username, :userpassword)");
    query.bindValue(":username", user->getName());
    query.bindValue(":userpassword", user->getName());
    if (!query.exec()) {
        qWarning() << "注册新用户时，插入User表失败！" << db.lastError().text();
        return false;
    }
    return true;
}

// 获取ID--------------------------------------------------------------------------
int UserDao::getIdByName(const QString &name) {
    QSqlDatabase db = DatabaseManager::instance().database();
    QSqlQuery query(db);
    query.prepare("SELECT user_id FROM User WHERE user_name = :username");
    query.bindValue(":username", name);

    if (!query.exec()) {
        qWarning() << "通过用户名查询id失败！" << db.lastError().text();
        return -1;
    }
    if (query.next()) {
        return query.value(0).toInt(); // 返回找到的user_id
    } else {
        qWarning() << "通过用户名查询id时，未找到用户:" << name;
        return -1; // 用户不存在
    }
}
