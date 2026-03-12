#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QMessageBox>
#include "user.h"

namespace Ui { class LoginDialog; }

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

signals:
    void userAuthenticated(User *user); // 用户登录成功

private slots:
    void registerUser(); // 注册用户
    void LoginUser();    // 登录用户

private:
    Ui::LoginDialog *ui;
};

#endif // LOGINDIALOG_H
