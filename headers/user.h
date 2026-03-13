#ifndef USER_H
#define USER_H

#include <QString>

class User {
public:
    User(const qint32& id, const QString& name, const QString& password);
    qint32 getId() const;
    QString getName() const;
    QString getPassword() const;

private:
    qint32 m_id;
    QString m_name;
    QString m_password;
};

#endif // USER_H
