#include "logindialog.h"
#include <QFile>
#include <QRegularExpression>
#include "ui_logindialog.h"
#include "userdao.h"

namespace
{
    /// @brief 验证用户名格式是否合法
    /// @details 用户名必须满足以下格式要求：
    ///          - 以字母开头
    ///          - 只能包含字母、数字和下划线
    ///          - 总长度在3-20位之间
    /// @param username 待验证的用户名字符串
    /// @return
    ///         - true  - 用户名格式合法
    ///         - false - 用户名格式不合法
    /// @note 该函数使用静态 QRegularExpression 对象以提高性能
    /// @see validatePassword()
    /// @see validateUsernameAndPassword()
    bool validateUsername(const QString& username)
    {
        // 用户名：字母开头，后面可以跟字母、数字、下划线，长度3-20
        static QRegularExpression re(R"(^[a-zA-Z][a-zA-Z0-9_]{2,19}$)");
        return re.match(username).hasMatch();
    }

    /// @brief 验证密码格式是否合法
    /// @details 密码必须满足以下格式要求：
    ///          - 只能包含字母和数字（允许纯字母或纯数字）
    ///          - 总长度在6-20位之间
    /// @param password 待验证的密码字符串
    /// @return
    ///         - true  - 密码格式合法
    ///         - false - 密码格式不合法
    /// @note 该函数使用静态 QRegularExpression 对象以提高性能
    /// @see validateUsername()
    /// @see validateUsernameAndPassword()
    bool validatePassword(const QString& password)
    {
        // 密码：6-20位，只能包含字母和数字（纯字母或纯数字也可）
        static QRegularExpression re(R"(^[a-zA-Z\d]{6,20}$)");
        return re.match(password).hasMatch();
    }

    /// @brief 综合验证用户名和密码，并在验证失败时显示错误对话框
    /// @details 依次验证用户名和密码的格式合法性。如果任一验证失败，
    ///          会显示相应的错误对话框并返回 false。该函数主要用于
    ///          登录和注册界面中的输入验证。
    ///
    /// @param username 待验证的用户名
    /// @param password 待验证的密码
    /// @param dialog 父对话框指针，用于显示错误消息框
    /// @param scene 操作场景描述（如"登录"或"注册"），用于错误对话框标题
    ///
    /// @return
    ///         - true  - 用户名和密码格式均合法
    ///         - false - 用户名或密码格式不合法（已显示错误提示）
    ///
    /// @warning 本函数会直接显示错误对话框，调用者无需再次提示
    /// @note 函数内部调用 validateUsername() 和 validatePassword() 进行具体验证
    ///
    /// @par 示例
    /// @code
    /// if (validateUsernameAndPassword(username, password, this, "登录")) {
    ///     // 执行登录逻辑
    /// }
    /// @endcode
    ///
    /// @see validateUsername()
    /// @see validatePassword()
    bool validateUsernameAndPassword(const QString& username, const QString& password,
                                            QDialog* dialog, const QString& scene)
    {
        if (!validateUsername(username))
        {
            QMessageBox::critical(dialog, QString("%1失败").arg(scene),
                    "用户名格式不正确！\n必须以字母开头，只能包含字母、数字和下划线，长度3-20位");
            return false;
        }
        if (!validatePassword(password))
        {
            QMessageBox::critical(dialog, QString("%1失败").arg(scene),
                    "密码格式不正确！\n只能包含字母和数字，长度6-20位");
            return false;
        }
        return true;
    }
} // namespace


LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);

    connect(ui->registerBtn, &QPushButton::clicked, this, &LoginDialog::registerUser);
    connect(ui->loginBtn, &QPushButton::clicked, this, &LoginDialog::loginUser);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::registerUser()
{
    const QString username = ui->usernameLineEdit->text();
    const QString password = ui->passwordLineEdit->text();
    if (!validateUsernameAndPassword(username, password, this, "注册"))
        return;

    if (UserDao::usernameExists(username))
    {
        QMessageBox::critical(this, "注册失败", "用户名已存在！");
        return;
    }

    User *user = new User(UserDao::getIdByName(username), username, password);
    if (UserDao::insertUser(user))
    {
        QMessageBox::information(this, "注册成功", "用户注册成功。");
        emit userAuthenticated(user);
        accept();
    }
    else
        QMessageBox::critical(this, "注册失败", "注册失败，请联系管理员！");
}

void LoginDialog::loginUser()
{
    const QString username = ui->usernameLineEdit->text();
    const QString password = ui->passwordLineEdit->text();
    if (!validateUsernameAndPassword(username, password, this, "登录"))
        return;

    if (UserDao::passwordCorrect(username, password))
    {
        User *user = new User(UserDao::getIdByName(username), username, password);
        emit userAuthenticated(user);
        accept();
    }
    else
        QMessageBox::critical(this, "登录失败", "密码错误！");
}
