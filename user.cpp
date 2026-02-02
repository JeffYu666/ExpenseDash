#include "user.h"
User::User(const int& id, const QString& name, const QString& password)
    : m_id(id)
    , m_name(name)
    , m_password(password) {}

int User::getId() const {
    return m_id;
}

QString User::getName() const {
    return m_name;
}

QString User::getPassword() const {
    return m_password;
}
