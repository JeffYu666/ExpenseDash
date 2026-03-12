#ifndef USER_H
#define USER_H

#include <QString>

class User {
public:
    User(const int& id, const QString& name, const QString& password);
    int getId() const;
    QString getName() const;
    QString getPassword() const;

private:
    int m_id;
    QString m_name;
    QString m_password;
};

#endif // USER_H
