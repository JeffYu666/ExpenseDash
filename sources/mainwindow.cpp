#include "mainwindow.h"
#include <QSqlError>
#include "chartwindow.h"
#include "databasemanager.h"
#include "searchdialog.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    displayVersion();// 显示应用程序版本信息
    setupToolBar();  // 初始化并设置主窗口的工具栏
}

void MainWindow::displayVersion()
{
    // 创建永久显示的版本信息
    QString version = QApplication::applicationVersion();
    QLabel *versionLabel = new QLabel(QString("version: v%1").arg(version), this);
    versionLabel->setFrameStyle(QFrame::NoFrame);

    // 添加到状态栏的永久区域（右侧）
    ui->statusbar->addPermanentWidget(versionLabel);
}

void MainWindow::addToolBarAction(const QString &text, const QString& iconPath,
                                  const QKeySequence::StandardKey& shortcut,
                                  void (MainWindow::*slot)())
{
    QAction *action = new QAction(text, this);
    if (!iconPath.isEmpty())
        action->setIcon(QIcon(iconPath));
    action->setShortcut(shortcut);
    connect(action, &QAction::triggered, this, slot);
    ui->toolBar->addAction(action);
}

void MainWindow::setupToolBar()
{
    // 创建工具栏
    ui->toolBar->setMovable(false);          // 固定工具栏位置
    ui->toolBar->setIconSize(QSize(24, 24)); // 设置图标大小

    // 新增功能
    addToolBarAction("新增", ":/icons/icons/add_circle.png",
                     QKeySequence::New, &MainWindow::onAddButtonClicked);
    // 删除功能
    addToolBarAction("删除", ":/icons/icons/delete.png",
                     QKeySequence::Delete, &MainWindow::onDeleteButtonClicked);
    // 提交功能
    addToolBarAction("提交", ":/icons/icons/upload.png",
                     QKeySequence::Save, &MainWindow::onSubmitButtonClicked);

    ui->toolBar->addSeparator();

    // 查询功能
    addToolBarAction("查询", ":/icons/icons/search.png",
                     QKeySequence::Find, &MainWindow::onSearchButtonClicked);

    ui->toolBar->addSeparator();

    // 图表和AI分析功能
    addToolBarAction("图表和AI分析", ":/icons/icons/finance.png",
                     QKeySequence::UnknownKey, &MainWindow::onChartButtonClicked);
}

void MainWindow::prepareTable()
{
    if (currentUser == nullptr)
    {
        QMessageBox::critical(this, "用户信息为空", "请联系管理员！");
        qFatal("MainWindow在prepareTable()中发现currentUser指向空！");
    }

    // 创建关系型表格模型
    model = new QSqlRelationalTableModel(this, DatabaseManager::getInstance().getDatabase());
    // 设置模型对应的数据库表为"Expense"
    model->setTable("Expense");

    // 设置分类关系（主要关系）
    model->setRelation(
        model->fieldIndex("category_id"),
        QSqlRelation("Category", "category_id", "category_name")
    );

    // 使用用户id进行过滤，只显示当前用户记录
    model->setFilter(QString("user_id = %1").arg(currentUser->getId()));
    // 设置为手动提交，需调用model->submitAll()才会提交
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    // 执行SQL查询，从数据库加载数据
    model->select();

    // 设置要显示的字段
    model->setHeaderData(model->fieldIndex("date"),          Qt::Horizontal, tr("日期"));
    model->setHeaderData(model->fieldIndex("category_name"), Qt::Horizontal, tr("分类"));
    model->setHeaderData(model->fieldIndex("amount"),        Qt::Horizontal, tr("金额"));
    model->setHeaderData(model->fieldIndex("description"),   Qt::Horizontal, tr("描述"));

    // -------------------------------------------------------------------------------

    // 将设置好的模型设置给tableView显示
    ui->tableView->setModel(model);
    // 设置委托，让分类字段显示为组合框
    ui->tableView->setItemDelegate(new QSqlRelationalDelegate(ui->tableView));
    // 设置选择行为：点击单元格时选中整行
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    // 启用交替行颜色
    ui->tableView->setAlternatingRowColors(true);
    // 设置水平表头的列宽调整模式，Stretch表示所有列自动拉伸以填满整个表格宽度
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 隐藏user_id，和expense_id字段
    ui->tableView->hideColumn(model->fieldIndex("user_id"));
    ui->tableView->hideColumn(model->fieldIndex("expense_id"));

    ui->tableView->setSortingEnabled(true);// 用户点击表头即可排序
    // 设置默认排序列为日期列，降序排列（最新的显示在最前面）
    ui->tableView->sortByColumn(
        model->fieldIndex("date"),
        Qt::DescendingOrder
    );
    // 将第3列移动到第1位置
    ui->tableView->horizontalHeader()->moveSection(3, 1);

    // 支持就地编辑：双击或edit键
    ui->tableView->setEditTriggers(
        QAbstractItemView::DoubleClicked |
        QAbstractItemView::EditKeyPressed
    );

    // 设置委托
    m_highlightDelegate = new HighlightDelegate(ui->tableView);
    ui->tableView->setItemDelegate(m_highlightDelegate);

    connect(model, &QSqlRelationalTableModel::dataChanged,
            this, &MainWindow::onModelDataChanged);
}

void MainWindow::onAddButtonClicked()
{
    if (model == nullptr)
    {
        QMessageBox::critical(this, tr("错误"), tr("数据模型未初始化！"));
        qFatal("MainWindow在onAddButtonClicked中发现model指向空！");
    }

    isInternalDataChange = true;

    // 在最后插入新行
    qint32 newRow = model->rowCount();
    if (!model->insertRow(newRow))
    {
        QMessageBox::critical(this, tr("错误"), tr("插入新行失败！"));
        return;
    }

    setDefaultValues(newRow);

    // 滚动到新行并开始编辑
    ui->tableView->scrollToBottom();
    QModelIndex dateIndex = model->index(newRow, model->fieldIndex("date"));
    ui->tableView->setCurrentIndex(dateIndex);
    ui->tableView->edit(dateIndex);

    isInternalDataChange = false;
}

void MainWindow::setDefaultValues(const qint32 newRow)
{
    if (!model)
        return;

    isInternalDataChange = true;  // 防止递归触发 dataChanged

    QSqlRecord record = model->record(newRow);

    record.setValue("user_id", currentUser->getId());
    record.setValue("date", QDate::currentDate().toString("yyyy-MM-dd"));
    record.setValue("amount", 0);

    model->setRecord(newRow, record);

    isInternalDataChange = false;
}

void MainWindow::onModelDataChanged(const QModelIndex &topLeft,
                        const QModelIndex &bottomRight,
                        const QList<int> &roles)
{
    Q_UNUSED(roles);

    if (isInternalDataChange)
        return;

    if (topLeft.column() != bottomRight.column() || topLeft.row() != bottomRight.row())
        qCritical() << "onModelDataChanged()中检测到存在多个数据项的改变";

    // qDebug() << "unValidateDataIndex inserting : (" << topLeft.row() << ", " << topLeft.column() << ")";

    unValidateDataIndex.insert(topLeft);
}

// TODO: 完善数据验证逻辑
bool MainWindow::validateCell(const QModelIndex& index)
{
    QString fieldName = model->headerData(index.column(), Qt::Horizontal).toString();
    QVariant value = model->data(index);

    if (fieldName == "日期")
    {
        if (value.isNull())
        {
            QMessageBox::warning(this, "输入错误", "日期不能为空！");
            return false;
        }
        QDate date = value.toDate();
        if (!date.isValid())
        {
            QMessageBox::warning(this, "输入错误", "日期格式无效！");
            return false;
        }
        return true;

    }
    else if (fieldName == "金额")
    {
        if (value.isNull())
        {
            QMessageBox::warning(this, "输入错误", "金额不能为空！");
            return false;
        }

        bool ok;
        double amount = value.toDouble(&ok);// TODO: 金额格式校验
        if (!ok)
        {
            QMessageBox::warning(this, "输入错误", "金额格式无效！");
            return false;
        }
        if (amount <= 0)
        {
            QMessageBox::warning(this, "输入错误", "金额需为正数！");
            return false;
        }

        // 精确的小数位数检查
        // 将金额转换为分（整数）来处理
        long long cents = std::round(amount * 100);
        long long integer_part = cents / 100;
        long long fractional_part = cents % 100;

        if (fractional_part > 0 && cents != integer_part * 100 + fractional_part)
        {
            QMessageBox::warning(this, "输入错误", "金额最多只能有2位小数！");
            return false;
        }
        return true;

    }
    else if (fieldName == "分类")
    {
        if (value.isNull())
        {
            QMessageBox::warning(this, "输入错误", "请选择支出分类！");
            return false;
        }
        return true;

    }
    else if (fieldName == "描述")
    {
        if (value.isNull())
            return true; // 描述可为空

        QString desc = value.toString();
        if (desc.length() > 50)
        {
            QMessageBox::warning(this, "输入错误", "描述长度不能超过50个字符！");
            return false;
        }
        return true;
    }
    else
        return true;
}

void MainWindow::onDeleteButtonClicked()
{
    // 获取当前选中的行
    QModelIndexList selectedIndexes = ui->tableView->selectionModel()->selectedRows();

    // 如果没有选中任何行，显示提示信息并返回
    if (selectedIndexes.isEmpty())
    {
        QMessageBox::warning(this, "删除操作", "请先选择要删除的行！");
        return;
    }

    isInternalDataChange = true;

    // 逆序删除避免行号变化导致错误
    std::sort(selectedIndexes.begin(),
              selectedIndexes.end(),
              [](const QModelIndex &a, const QModelIndex &b)
    {
        return a.row() > b.row();
    });

    for (const QModelIndex &index : selectedIndexes)
    {
        model->removeRow(index.row());
    }

    isInternalDataChange = false;

    QMessageBox::information(this, "删除操作", "点击提交，彻底删除。");
}

void MainWindow::highlightCell(const QModelIndex &index, bool isError)
{
    if (!index.isValid())
        return;

    isInternalDataChange = true;// 防止递归触发 dataChanged


    if (isError)
        m_highlightDelegate->addCell(QPersistentModelIndex(index));
    else
        m_highlightDelegate->removeCell(QPersistentModelIndex(index));

    ui->tableView->update(index);// 只刷新这个单元格

    isInternalDataChange = false;
}

void MainWindow::onSubmitButtonClicked()
{
    if (!model)
        return;

    // qDebug() << "修改过的单元格数量：" << unValidateDataIndex.size();

    // 提交前对修改过的单元格逐一验证
    for (auto it = unValidateDataIndex.begin(); it != unValidateDataIndex.end(); )
    {
        if (!validateCell(*it))
        {
            highlightCell(*it, true);// 验证失败，高亮
            return;
        }
        else
        {
            highlightCell(*it, false);// 验证通过，取消高亮
            it = unValidateDataIndex.erase(it);
        }
    }

    // 提交到数据库
    if (model->submitAll())
        QMessageBox::information(this, "成功", "数据已保存！");
    else
        QMessageBox::critical(this, "错误", QString("提交失败: %1").arg(model->lastError().text()));
}

void MainWindow::onSearchButtonClicked()
{
    SearchDialog searchDialog(model, currentUser, this);
    if (searchDialog.exec() == QDialog::Accepted)
        QMessageBox::information(this, "查询", "查询成功");
}

void MainWindow::onChartButtonClicked()
{
    chartWindow = new ChartWindow(this, currentUser);
    chartWindow->show();
}

void MainWindow::setCurrentUser(const User *user)
{
    currentUser = user;
}

MainWindow::~MainWindow()
{
    delete ui;
    delete currentUser;
    delete model;
}
