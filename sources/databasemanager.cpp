#include "databasemanager.h"
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QSqlError>

DatabaseManager& DatabaseManager::getInstance()
{
    static DatabaseManager instance;
    return instance;
}

void DatabaseManager::initializeDatabase(const QString& relativeDirName)
{
    if (m_initialized)
        return;

    QString appDirPath = QApplication::applicationDirPath();
    // appDir/data
    QString absoluteDirPath = appDirPath + QDir::separator() + relativeDirName;

    QDir dir(appDirPath);
    if (!dir.mkpath(relativeDirName))
    {
        QString errorStr = QString("创建数据目录失败！\n路径: %1\n错误: %2")
                               .arg(absoluteDirPath)
                               .arg(strerror(errno));

        qFatal("%s", qPrintable(errorStr));
    }

    // appDir/data/database.db
    QString absoluteDbPath = absoluteDirPath + QDir::separator() + "database.db";

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(absoluteDbPath);

    m_initialized = m_db.open();
    if (m_initialized)
    {
        qInfo().noquote() << QString("数据库初始化成功\n"
                                     "    路径: %1\n"
                                     "    驱动: %2")
                              .arg(absoluteDbPath)
                              .arg(m_db.driverName());
    }
    else
    {
        QSqlError error = m_db.lastError();
        QString errorMsg = QString("数据库初始化失败！\n"
                                   "    路径: %1\n"
                                   "    错误: %2\n"
                                   "    类型: %3\n"
                                   "    驱动错误: %4")
                               .arg(absoluteDbPath)
                               .arg(error.text())
                               .arg(error.type())
                               .arg(error.driverText());

        qFatal("%s", qPrintable(errorMsg));
    }
}

bool DatabaseManager::isInitialized()
{
    return m_initialized;
}

QSqlDatabase& DatabaseManager::getDatabase()
{
    if (!m_initialized)
        qFatal("尝试获取未初始化的数据库连接！");
    return m_db;
}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen())
        m_db.close();
}
