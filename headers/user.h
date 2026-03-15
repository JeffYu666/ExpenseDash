#ifndef USER_H
#define USER_H

#include <QString>

/// @brief 用户类，存储用户信息和处理密码加密
///
/// User类负责管理用户的基本信息，包括用户ID、用户名、
/// 密码哈希值和盐值。密码使用SHA256加盐哈希存储，确保安全性。
class User
{
public:
    /// @brief 构造函数，创建新用户
    /// @param id 用户ID
    /// @param name 用户名，用于登录标识
    /// @param password 明文密码，会被自动加盐哈希存储
    ///
    /// @see generateSalt(), hashPassword()
    User(const qint32& id, const QString& name, const QString& password);

    /// @brief 获取用户ID
    /// @return 用户ID
    qint32 getId() const;

    /// @brief 获取用户名
    /// @return 用户名
    QString getName() const;

    /// @brief 获取密码哈希值
    /// @return SHA256哈希值的十六进制字符串
    QString getHash() const;

    /// @brief 获取密码盐值
    /// @return 用于密码哈希的随机盐值
    QString getSalt() const;

    /// @brief 静态方法：计算密码加盐哈希
    /// @param password 明文密码
    /// @param salt 盐值字符串
    /// @return SHA256(password + salt)的十六进制字符串
    static QString hashPassword(const QString& password, const QString& salt);

private:
    qint32 m_id;   ///< 用户唯一标识ID
    QString m_name;///< 用户名，用于登录
    QString m_hash;///< 密码哈希值，由hashPassword()生成
    QString m_salt;///< 随机生成的盐值
};

#endif // USER_H
