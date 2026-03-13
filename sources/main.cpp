#include <QApplication>
#include "logindialog.h"
#include "mainwindow.h"
#include "databasemanager.h"
#include "userdao.h"

/// @brief 设置应用程序的样式
/// @param app 应用程序对象
void setApplicationStyle(QApplication& app);

/// @brief 完成数据库的所有准备工作
/// @details 准备工作包括：初始化数据库,启用外键约束,创建所有表,创建所有索引,插入测试数据。
void prepareDatabase();

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("个人收支分析系统");
    app.setApplicationVersion("1.2.0");

    // 设置应用程序的样式
    setApplicationStyle(app);

    // LoginDialog中registerUser和LoginUser需访问数据库
    // 数据库的所有准备工作需在LoginDialog创建前完成
    prepareDatabase();

    LoginDialog loginDialog;
    MainWindow mainWindow;

    // 连接登录对话框的用户认证信号到主窗口的设置当前用户槽函数
    QObject::connect(&loginDialog,
                     &LoginDialog::userAuthenticated,
                     &mainWindow,
                     &MainWindow::setCurrentUser);
    
    // 在登录对话框中用户认证失败，则直接退出应用程序
    if (loginDialog.exec() != QDialog::Accepted)
        return 0;

    mainWindow.prepareTable();
    mainWindow.show();

    return app.exec();
}

void setApplicationStyle(QApplication& app)
{
    QFile file(":/style/style.qss");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning().noquote() << "读取style.qss样式文件出错！" << file.errorString();
        return;
    }

    QTextStream filetext(&file);
    filetext.setEncoding(QStringConverter::Utf8);// style.qss编码为Utf8

    QString stylesheet = filetext.readAll();
    // 检查读取是否成功
    if (filetext.status() != QTextStream::Ok)
    {
        qWarning().noquote() << "读取样式文件内容失败！";
        return;
    }

    // 应用样式
    if (!stylesheet.isEmpty())
    {
        app.setStyleSheet(stylesheet);
        qInfo().noquote() << "样式文件加载成功。";
    }

    file.close();
}

void prepareDatabase()
{
    DatabaseManager& manager = DatabaseManager::getInstance();
    manager.initializeDatabase();// 初始化数据库

    UserDao::enableForeignKeys();// 启用外键约束
    UserDao::createAllTables();  // 创建所有表
    UserDao::createIndexes();    // 创建所有索引
    UserDao::insertTestData();   // 插入测试数据
    qInfo().noquote() << "数据库所有准备工作已完成。";
}
