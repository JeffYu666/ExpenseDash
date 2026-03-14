#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QMessageBox>
#include "user.h"

namespace Ui { class LoginDialog; }

/// @brief 用户登录/注册对话框类
/// @details 提供用户认证的图形界面，支持登录和注册两种功能。
///          用户通过输入用户名和密码进行身份验证，验证成功后
///          通过 userAuthenticated 信号传递认证信息。
///
/// @par 功能特性
/// - 用户登录：验证用户名和密码的匹配性
/// - 用户注册：创建新的用户账户
/// - 输入验证：实时验证用户名和密码格式
/// - 错误提示：友好的用户反馈机制
///
/// @see UserDao
/// @see User
class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

signals:
    void userAuthenticated(User *user); // 用户登录成功

private slots:
    /// @brief 处理用户注册的槽函数
    /// @details 当用户点击注册按钮时触发。
    ///
    /// @note 这是一个私有槽函数，通过信号-槽机制与注册按钮的clicked信号连接
    ///
    /// @warning 函数会动态创建User对象，所有权转移给接收userAuthenticated信号的组件
    ///
    /// @see loginUser()
    /// @see validateUsernameAndPassword()
    /// @see UserDao::usernameExists()
    /// @see UserDao::insertUser()
    /// @see userAuthenticated()
    void registerUser();

    /// @brief 处理用户登录的槽函数
    /// @details 当用户点击登录按钮时触发。
    ///
    /// @note 这是一个私有槽函数，通过信号-槽机制与登录按钮的clicked信号连接
    ///
    /// @warning 函数会动态创建User对象，所有权转移给接收userAuthenticated信号的组件
    ///          调用者负责管理User对象的生命周期
    ///
    /// @attention 密码验证使用UserDao::passwordCorrect()进行，该函数应对密码进行
    ///            哈希比较，不应以明文形式处理密码
    ///
    /// @see registerUser()
    /// @see validateUsernameAndPassword()
    /// @see UserDao::passwordCorrect()
    /// @see UserDao::getIdByName()
    /// @see userAuthenticated()
    void loginUser();

private:
    Ui::LoginDialog *ui;
};

#endif // LOGINDIALOG_H
