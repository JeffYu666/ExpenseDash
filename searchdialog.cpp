#include "searchdialog.h"
#include <QDate>
#include <QDebug>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include "ui_searchdialog.h"
#include "userdao.h"

SearchDialog::SearchDialog(QSqlRelationalTableModel *model, const User *currentUser, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SearchDialog)
    , m_model(model)
    , m_currentUser(currentUser) {
    ui->setupUi(this);

    // 设置日期范围
    setupDateRange();

    // 填充类别组合框
    populateCategoryComboBox();

    // 设置金额范围初始值
    ui->minAmountSpinBox->setMinimum(0);
    ui->minAmountSpinBox->setMaximum(999999.99);
    ui->minAmountSpinBox->setValue(0);

    ui->maxAmountSpinBox->setMinimum(0);
    ui->maxAmountSpinBox->setMaximum(999999.99);
    ui->maxAmountSpinBox->setValue(1000); // 默认最大1000

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SearchDialog::onButtonBoxAccepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &SearchDialog::onButtonBoxRejected);
}

SearchDialog::~SearchDialog() {
    delete ui;
}

void SearchDialog::setupDateRange() {
    // 设置默认日期范围为当前月份
    QDate currentDate = QDate::currentDate();
    QDate startDate(currentDate.year(), currentDate.month(), 1);
    QDate endDate = startDate.addMonths(1).addDays(-1);

    ui->startDateEdit->setDate(startDate);
    ui->endDateEdit->setDate(endDate);
}

void SearchDialog::populateCategoryComboBox() {
    ui->categoryComboBox->clear();
    ui->categoryComboBox->addItem(tr("所有分类"), -1); // 添加"所有分类"选项

    // 从数据库获取所有分类
    QSqlQuery query("SELECT category_id, category_name FROM Category ORDER BY category_id");
    while (query.next()) {
        int categoryId = query.value(0).toInt();
        QString categoryName = query.value(1).toString();
        ui->categoryComboBox->addItem(categoryName, categoryId);
    }
}

void SearchDialog::onButtonBoxAccepted() {
    // 构建过滤条件
    QStringList filters;

    // 当前用户过滤
    filters.append(QString("user_id = %1").arg(m_currentUser->getId()));

    // 日期范围过滤
    QDate startDate = ui->startDateEdit->date();
    QDate endDate = ui->endDateEdit->date();
    filters.append(QString("date(date) BETWEEN date('%1') AND date('%2')")
                       .arg(startDate.toString("yyyy-MM-dd"))
                       .arg(endDate.toString("yyyy-MM-dd")));

    // 分类过滤
    int categoryId = ui->categoryComboBox->currentData().toInt();
    if (categoryId != -1) { // -1 表示"所有分类"
        filters.append(QString("Expense.category_id = %1").arg(categoryId));
    }

    // 金额范围过滤
    double minAmount = ui->minAmountSpinBox->value();
    double maxAmount = ui->maxAmountSpinBox->value();
    if (minAmount > 0 || maxAmount > 0) {
        if (minAmount > 0 && maxAmount > 0) {
            filters.append(QString("amount BETWEEN %1 AND %2").arg(minAmount).arg(maxAmount));
        } else if (minAmount > 0) {
            filters.append(QString("amount >= %1").arg(minAmount));
        } else if (maxAmount > 0) {
            filters.append(QString("amount <= %1").arg(maxAmount));
        }
    }

    // 调试输出
    QString filterStr = filters.join(" AND ");

    // 应用过滤条件
    m_model->setFilter(filterStr);
    bool selectSuccess = m_model->select();

    if (!selectSuccess) {
        QMessageBox::warning(this, tr("错误"), tr("过滤失败: %1").arg(m_model->lastError().text()));
        reject();
    } else {
        accept();
    }
}

void SearchDialog::onButtonBoxRejected() {
    reject();
}
