# Qt模块配置
QT += core gui sql charts network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# 构建配置
CONFIG += c++17

# 包含路径
INCLUDEPATH += headers

# 编译器选项
# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # 禁用Qt 6.0.0之前的API

# 源文件
SOURCES += \
    sources/chartwindow.cpp \
    sources/databasemanager.cpp \
    sources/logindialog.cpp \
    sources/main.cpp \
    sources/mainwindow.cpp \
    sources/searchdialog.cpp \
    sources/user.cpp \
    sources/userdao.cpp

# 头文件
HEADERS += \
    headers/chartwindow.h \
    headers/databasemanager.h \
    headers/logindialog.h \
    headers/mainwindow.h \
    headers/searchdialog.h \
    headers/user.h \
    headers/userdao.h

# UI文件
FORMS += \
    forms/chartwindow.ui \
    forms/logindialog.ui \
    forms/mainwindow.ui \
    forms/searchdialog.ui

# 资源文件
RESOURCES += \
    resources/res.qrc

# 部署规则
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
