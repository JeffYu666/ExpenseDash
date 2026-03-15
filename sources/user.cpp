#include "user.h"
#include <QCryptographicHash>
#include <QRandomGenerator>

QString User::hashPassword(const QString& password, const QString& salt)
{
    // 组合密码和盐
    QByteArray data = (password + salt).toUtf8();

    // 计算SHA256哈希
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);

    // 存储十六进制字符串
    return QString(hash.toHex());
}

namespace
{
    /// @brief 生成随机盐值
    /// @param length 生成的盐值长度，默认为16
    /// @return 包含随机字符的盐值字符串
    /// @note 使用QRandomGenerator生成安全的随机字符
    QString generateSalt(int length = 16)
    {
        const QString possibleChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

        QString salt;
        salt.reserve(length);

        QRandomGenerator *generator = QRandomGenerator::global();

        for (int i = 0; i < length; ++i) {
            // 生成随机索引
            int index = generator->bounded(possibleChars.length());
            salt.append(possibleChars.at(index));
        }

        return salt;
    }
}// namespace

User::User(const qint32& id, const QString& name, const QString& password)
    : m_id(id)
    , m_name(name)
{
    m_salt = generateSalt();
    m_hash = User::hashPassword(password, m_salt);
}

qint32 User::getId() const
{
    return m_id;
}
QString User::getName() const
{
    return m_name;
}
QString User::getHash() const
{
    return m_hash;
}
QString User::getSalt() const
{
    return m_salt;
}

