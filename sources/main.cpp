#include <QApplication>
#include "logindialog.h"
#include "mainwindow.h"

void setStyle(QApplication& app);

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("个人收支分析系统");
    app.setApplicationVersion("1.2.0");

    setStyle(app);

    LoginDialog loginDialog;
    MainWindow mainWindow;

    // 连接信号槽
    QObject::connect(&loginDialog,
                     &LoginDialog::userAuthenticated,
                     &mainWindow,
                     &MainWindow::setCurrentUser);

    if (loginDialog.exec() != QDialog::Accepted) {
        return 0;
    }

    mainWindow.prepareTable();
    mainWindow.show();

    return app.exec();
}

void setStyle(QApplication& app) {
    QFile file(":/style/style.qss");
    file.open(QFile::ReadOnly);
    QTextStream filetext(&file);
    QString stylesheet = filetext.readAll();
    app.setStyleSheet(stylesheet);
    file.close();
}
