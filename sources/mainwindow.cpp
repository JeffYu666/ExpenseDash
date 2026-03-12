#include "mainwindow.h"
#include <QSqlError>
#include "chartwindow.h"
#include "databasemanager.h"
#include "searchdialog.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setupToolBar();

    // 创建永久显示的版本信息
    QString version = QApplication::applicationVersion();
    QLabel *versionLabel = new QLabel(QString("Version: v%1").arg(version), this);
    versionLabel->setFrameStyle(QFrame::NoFrame);
    versionLabel->setIndent(5); // 添加一些缩进

    // 添加到状态栏的永久区域（右侧）
    ui->statusbar->addPermanentWidget(versionLabel);
}

void MainWindow::setupToolBar() {
    // 创建工具栏
    ui->toolBar->setMovable(false);          // 固定工具栏位置
    ui->toolBar->setIconSize(QSize(24, 24)); // 设置图标大小

    // 新增功能
    QAction *addAction = new QAction(tr("新增"), this);
    addAction->setIcon(QIcon(":/icons/icons/add_circle.png"));
    addAction->setShortcut(QKeySequence::New);
    connect(addAction, &QAction::triggered, this, &MainWindow::onAddButtonClicked);
    ui->toolBar->addAction(addAction);

    // 删除功能
    QAction *deleteAction = new QAction(tr("删除"), this);
    deleteAction->setIcon(QIcon(":/icons/icons/delete.png"));
    deleteAction->setShortcut(QKeySequence::Delete);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteButtonClicked);
    ui->toolBar->addAction(deleteAction);

    // 提交功能
    QAction *submitAction = new QAction(tr("提交"), this);
    submitAction->setIcon(QIcon(":/icons/icons/upload.png"));
    submitAction->setShortcut(QKeySequence::Save);
    connect(submitAction, &QAction::triggered, this, &MainWindow::onSubmitButtonClicked);
    ui->toolBar->addAction(submitAction);

    ui->toolBar->addSeparator();

    // 查询功能
    QAction *searchAction = new QAction(tr("查询"), this);
    searchAction->setIcon(QIcon(":/icons/icons/search.png"));
    searchAction->setShortcut(QKeySequence::Find);
    connect(searchAction, &QAction::triggered, this, &MainWindow::onSearchButtonClicked);
    ui->toolBar->addAction(searchAction);

    ui->toolBar->addSeparator();

    // 图表分析功能
    QAction *chartAction = new QAction(tr("图表和AI分析"), this);
    chartAction->setIcon(QIcon(":/icons/icons/finance.png"));
    connect(chartAction, &QAction::triggered, this, &MainWindow::onChartButtonClicked);
    ui->toolBar->addAction(chartAction);
}

void MainWindow::prepareTable() {
    if (currentUser == nullptr) {
        QMessageBox::critical(this, "用户信息为空", "请联系管理员！");
    }

    // 清理旧的model
    if (model) {
        delete model;
        model = nullptr;
    }

    model = new QSqlRelationalTableModel(this, DatabaseManager::instance().database());
    model->setTable("Expense");

    // 设置分类关系（主要关系）
    model->setRelation(model->fieldIndex("category_id"),
                       QSqlRelation("Category", "category_id", "category_name"));

    // 设置用户过滤（但不在关系中显示用户名）
    model->setFilter(QString("user_id = %1").arg(currentUser->getId()));
    model->setSort(model->fieldIndex("date"), Qt::DescendingOrder);
    // 设置要显示的字段
    model->setHeaderData(model->fieldIndex("date"), Qt::Horizontal, tr("日期"));
    model->setHeaderData(model->fieldIndex("category_id"), Qt::Horizontal, tr("分类"));
    model->setHeaderData(model->fieldIndex("amount"), Qt::Horizontal, tr("金额"));
    model->setHeaderData(model->fieldIndex("description"), Qt::Horizontal, tr("描述"));

    model->setEditStrategy(QSqlTableModel::OnManualSubmit); // 设置为手动提交
    // 选择数据
    model->select();

    // 设置委托，让分类字段显示为组合框
    ui->tableView->setItemDelegate(new QSqlRelationalDelegate(ui->tableView));

    ui->tableView->setModel(model);

    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setAlternatingRowColors(true);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 隐藏不需要的字段（如user_id、balance_id等）
    ui->tableView->hideColumn(model->fieldIndex("expense_id"));
    ui->tableView->hideColumn(model->fieldIndex("user_id"));

    // 设置列显示顺序
    // 允许用户拖动列来重新排序
    ui->tableView->setSortingEnabled(true);

    // 或者以编程方式移动列
    ui->tableView->horizontalHeader()->moveSection(3, 1); // 将第3列移动到第1位置

    // 支持就地编辑
    ui->tableView
        ->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

    // 改为连接dataChanged信号
    connect(model, &QSqlTableModel::dataChanged, this, &MainWindow::onDataChanged);
}

void MainWindow::onAddButtonClicked() {
    if (!model) {
        QMessageBox::critical(this, tr("错误"), tr("数据模型未初始化！"));
        return;
    }

    // 在最后插入新行
    int newRow = model->rowCount();
    if (!model->insertRow(newRow)) {
        QMessageBox::critical(this, tr("错误"), tr("插入新行失败！"));
        return;
    }

    setDefaultValues(newRow);

    // 滚动到新行并开始编辑
    ui->tableView->scrollToBottom();
    QModelIndex dateIndex = model->index(newRow, model->fieldIndex("date"));
    ui->tableView->setCurrentIndex(dateIndex);
    ui->tableView->edit(dateIndex);
}

void MainWindow::setDefaultValues(const int newRow) {
    if (!model)
        return;

    isProgrammaticChange = true;

    QSqlRecord record = model->record(newRow);

    record.setValue("user_id", currentUser->getId());
    record.setValue("date", QDate::currentDate().toString("yyyy-MM-dd"));
    record.setValue("category_id", 1); // 默认分类
    record.setValue("amount", 0);

    model->setRecord(newRow, record);
    // QModelIndex idx = model->index(newRow, model->fieldIndex("category_id"));
    // qDebug() << "DisplayRole:" << model->data(idx, Qt::DisplayRole).toString();
    // qDebug() << "EditRole:" << model->data(idx, Qt::EditRole).toInt();

    isProgrammaticChange = false;
}

void MainWindow::onDataChanged(const QModelIndex &topLeft,
                               const QModelIndex &bottomRight,
                               const QVector<int> &roles) {
    if (!model || isProgrammaticChange)
        return;

    // 遍历所有发生变化的单元格
    for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
        for (int col = topLeft.column(); col <= bottomRight.column(); ++col) {
            QModelIndex index = model->index(row, col);

            // 实时验证单个单元格
            validateCell(row, col, model->data(index));
        }
    }
}

bool MainWindow::validateCell(int row, int column, const QVariant &value) {
    Q_UNUSED(row);
    QString fieldName = model->headerData(column, Qt::Horizontal).toString();

    if (fieldName == tr("日期")) {
        if (value.isNull()) {
            QMessageBox::warning(this, "输入错误", "日期不能为空！");
            return false;
        }
        QDate date = value.toDate();
        if (!date.isValid()) {
            QMessageBox::warning(this, "输入错误", "日期格式无效！");
            return false;
        }
        return true;

    } else if (fieldName == tr("金额")) {
        if (value.isNull() || value.toInt() <= 0) {
            QMessageBox::warning(this, "输入错误", "金额不能为空！");
            return false;
        }

        bool ok;
        double amount = value.toDouble(&ok);
        if (!ok) {
            QMessageBox::warning(this, "输入错误", "金额格式无效！");
            return false;
        }
        if (amount <= 0) {
            QMessageBox::warning(this, "输入错误", "金额必须大于0！");
            return false;
        }

        // 精确的小数位数检查
        // 将金额转换为分（整数）来处理
        long long cents = std::round(amount * 100);
        long long integer_part = cents / 100;
        long long fractional_part = cents % 100;

        if (fractional_part > 0 && cents != integer_part * 100 + fractional_part) {
            QMessageBox::warning(this, "输入错误", "金额最多只能有2位小数！");
            return false;
        }
        return true;

    } else if (fieldName == tr("分类")) {
        if (value.isNull()) {
            QMessageBox::warning(this, "输入错误", "请选择支出分类！");
            return false;
        }
        return true;

    } else if (fieldName == tr("描述")) {
        if (value.isNull())
            return true; // 描述可为空

        QString desc = value.toString();
        if (desc.length() > 50) {
            QMessageBox::warning(this, "输入错误", "描述长度不能超过50个字符！");
            return false;
        }
        return true;
    } else {
        return true;
    }
}

void MainWindow::onDeleteButtonClicked() {
    // 获取当前选中的行
    QModelIndexList selectedIndexes = ui->tableView->selectionModel()->selectedRows();

    // 如果没有选中任何行，显示提示信息并返回
    if (selectedIndexes.isEmpty()) {
        QMessageBox::warning(this, "删除操作", "请先选择要删除的行！");
        return;
    }

    // 逆序删除避免行号变化导致错误
    std::sort(selectedIndexes.begin(),
              selectedIndexes.end(),
              [](const QModelIndex &a, const QModelIndex &b) { return a.row() > b.row(); });

    for (const QModelIndex &index : selectedIndexes) {
        model->removeRow(index.row());
    }

    QMessageBox::information(this, "删除操作", "点击提交，彻底删除。");
}

void MainWindow::onSubmitButtonClicked() {
    if (!model)
        return;

    // 验证所有行
    for (int row = 0; row < model->rowCount(); ++row) {
        if (!validateRow(row)) {
            QMessageBox::warning(this,
                                 tr("验证失败"),
                                 tr("第%1行数据验证失败，请修正后重试").arg(row + 1));
            return;
        }
    }

    // 提交到数据库
    if (model->submitAll()) {
        QMessageBox::information(this, tr("成功"), tr("数据已保存！"));
    } else {
        QMessageBox::critical(this, tr("错误"), tr("提交失败: %1").arg(model->lastError().text()));
    }
}

bool MainWindow::validateRow(int row) {
    // 验证可见的必填字段
    QVector<int> columnsToValidate;

    // 添加需要验证的列索引
    columnsToValidate << model->fieldIndex("date") << model->fieldIndex("amount")
                      << model->fieldIndex("category_id");
    // 不验证隐藏的 user_id，程序设置

    for (int col : columnsToValidate) {
        QModelIndex index = model->index(row, col);
        QVariant value = model->data(index);

        if (!validateCell(row, col, value)) {
            return false;
        }
    }

    return true;
}

void MainWindow::onSearchButtonClicked() {
    SearchDialog searchDialog(model, currentUser, this);
    if (searchDialog.exec() == QDialog::Accepted) {
        QMessageBox::information(this, "查询", "查询成功");
    }
}

void MainWindow::onChartButtonClicked() {
    // ChartWindow
    chartWindow = new ChartWindow(currentUser, this);
    chartWindow->show();
}

void MainWindow::setCurrentUser(const User *user) {
    currentUser = user;
}

MainWindow::~MainWindow() {
    delete ui;
    delete currentUser;
    delete model;
}
