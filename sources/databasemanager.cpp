#include "databasemanager.h"
#include <QApplication>
#include <QDebug>
#include <QDir>

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

void DatabaseManager::initialize(const QString& dbPath) {
    if (m_initialized)
        return;

    QString actualDbPath = dbPath;
    if (actualDbPath.isEmpty()) {
        QString appDir = QApplication::applicationDirPath();
        QString dataDir = appDir + "/data";

        QDir dir(dataDir);
        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                qWarning() << "无法创建数据目录:" << dataDir;
                return;
            }
        }
        actualDbPath = dataDir + "/database.db";
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(actualDbPath);

    m_initialized = m_db.open();
    if (m_initialized) {
        qDebug() << "数据库初始化成功。";
    } else {
        qWarning() << "数据库初始化失败！";
    }
}

QSqlDatabase& DatabaseManager::database() {
    return m_db;
}

bool DatabaseManager::isInitialized() const {
    return m_initialized;
}

DatabaseManager::~DatabaseManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
}
