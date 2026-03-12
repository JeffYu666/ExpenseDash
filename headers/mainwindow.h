#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QButtonGroup>
#include <QDate>
#include <QDebug>
#include <QMainWindow>
#include <QMessageBox>
#include <QSqlRecord>
#include <QSqlRelationalDelegate>
#include <QSqlRelationalTableModel>
#include <QVector>
#include "chartwindow.h"
#include "user.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void prepareTable();

public slots:
    void setCurrentUser(const User *user);
    void onAddButtonClicked();
    void onDeleteButtonClicked();
    void onSubmitButtonClicked();
    void onSearchButtonClicked();
    void onChartButtonClicked();

    void onDataChanged(const QModelIndex &topLeft,
                       const QModelIndex &bottomRight,
                       const QVector<int> &roles);

private:
    Ui::MainWindow *ui;
    const User *currentUser = nullptr;
    QSqlRelationalTableModel *model = nullptr;

    ChartWindow *chartWindow = nullptr;

    bool isProgrammaticChange = false;

    void setupToolBar();
    void setDefaultValues(const int newRow);
    int getFirstCategoryId();

    bool validateAndSubmitRow(int row);

    bool validateCell(int row, int column, const QVariant &value);
    bool validateRow(int row); // 验证用户是否存在

    bool validateCategoryExists(int categoryId); // 验证分类是否存在
};
#endif // MAINWINDOW_H
