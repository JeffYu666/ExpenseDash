#include "logindialog.h"
#include <QFile>
#include "databasemanager.h"
#include "ui_logindialog.h"
#include "userdao.h"

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog) {
    ui->setupUi(this);

    DatabaseManager::instance().initialize(); // 初始化数据库
    UserDao::enableForeignKeys();             // 启用外键约束
    UserDao::createAllTables();               // 创建所有表
    UserDao::createIndexes();                 // 创建所有索引
    UserDao::insertTestData();                // 插入测试数据

    connect(ui->registerBtn, &QPushButton::clicked, this, &LoginDialog::registerUser);
    connect(ui->loginBtn, &QPushButton::clicked, this, &LoginDialog::LoginUser);
}

LoginDialog::~LoginDialog() {
    delete ui;
}

void LoginDialog::registerUser() {
    const QString username = ui->usernameLineEdit->text();
    const QString password = ui->passwordLineEdit->text();
    if (username.isEmpty()) {
        QMessageBox::critical(this, "注册失败", "用户名为空！");
        return;
    }
    if (password.isEmpty()) {
        QMessageBox::critical(this, "注册失败", "密码为空！");
        return;
    }
    if (UserDao::usernameExists(username)) {
        QMessageBox::critical(this, "注册失败", "用户名已存在！");
        return;
    }
    User *user = new User(UserDao::getIdByName(username), username, password);
    if (UserDao::insertUser(user)) {
        QMessageBox::information(this, "注册成功", "用户注册成功。");
        emit userAuthenticated(user);
        accept();
    } else {
        QMessageBox::critical(this, "注册失败", "注册失败，请联系管理员！");
    }
}

void LoginDialog::LoginUser() {
    const QString username = ui->usernameLineEdit->text();
    const QString password = ui->passwordLineEdit->text();
    if (username.isEmpty()) {
        QMessageBox::critical(this, "登录失败", "用户名为空！");
        return;
    }
    if (password.isEmpty()) {
        QMessageBox::critical(this, "登录失败", "密码为空！");
        return;
    }

    if (UserDao::passwordCorrect(username, password)) {
        User *user = new User(UserDao::getIdByName(username), username, password);
        emit userAuthenticated(user);
        accept();
    } else {
        QMessageBox::critical(this, "登录失败", "密码错误！");
    }
}
