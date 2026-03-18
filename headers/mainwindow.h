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
#include <QSet>
#include "chartwindow.h"
#include "user.h"

/// @brief 自定义高亮委托类，用于在表格中高亮显示特定单元格
///
/// 该类继承自QStyledItemDelegate，通过维护一个持久化模型索引的集合
/// 来标记需要高亮显示的单元格，并在绘制时为其设置红色背景。
class HighlightDelegate : public QStyledItemDelegate
{
private:
    QSet<QPersistentModelIndex> highlightedCells;///< 存储需要高亮的单元格的持久化索引

public:
    HighlightDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {}

    /// @brief 添加需要高亮的单元格
    /// @param index 需要高亮的单元格的持久化索引
    void addCell(const QPersistentModelIndex &index)
    {
        highlightedCells.insert(index);
    }
    /// @brief 移除单元格的高亮
    /// @param index 需要移除高亮的单元格的持久化索引
    void removeCell(const QPersistentModelIndex &index)
    {
        highlightedCells.remove(index);
    }

    /// @brief 重绘单元格
    /// @param painter 绘图设备
    /// @param option 绘制选项
    /// @param index 当前绘制的单元格索引
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        if (highlightedCells.contains(QPersistentModelIndex(index)))
        {

            // qDebug() << "Painting : (" << index.row() << ", " << index.column() << ")";

            // 先手动填充背景
            painter->save();
            painter->fillRect(opt.rect, QColor(255, 200, 200));
            painter->restore();

            // 清除默认背景绘制
            opt.backgroundBrush = Qt::NoBrush;
        }

        // 绘制文本等内容
        QStyledItemDelegate::paint(painter, opt, index);
    }
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/// @brief 主窗口类，提供消费记录管理的主要界面
///
/// 该类负责显示用户的消费记录表格，提供增删改查、数据验证、
/// 图表分析等功能。通过QSqlRelationalTableModel与数据库交互，
/// 使用自定义委托实现单元格高亮显示。
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    /// @brief 准备并初始化表格模型
    ///
    /// 创建关系型表格模型，设置表关联、过滤条件、显示字段等，
    /// 并将模型设置给tableView。同时设置委托、选择行为等界面属性。
    void prepareTable();

public slots:
    /// @brief 设置当前用户
    /// @param user 当前登录的用户对象指针
    void setCurrentUser(const User *user);

    /// @brief 处理新增按钮点击事件
    ///
    /// 在表格末尾插入新行，设置默认值（用户ID、当前日期、金额0），
    /// 并自动进入编辑状态。
    void onAddButtonClicked();

    /// @brief 处理删除按钮点击事件
    ///
    /// 删除当前选中的行（支持多选），删除操作在提交前不会影响数据库。
    void onDeleteButtonClicked();

    /// @brief 处理提交按钮点击事件
    ///
    /// 验证所有修改过的单元格数据，验证通过后将更改提交到数据库。
    void onSubmitButtonClicked();

    /// @brief 处理查询按钮点击事件
    ///
    /// 打开查询对话框，允许用户按条件筛选记录。
    void onSearchButtonClicked();

    /// @brief 处理图表分析按钮点击事件
    ///
    /// 打开图表分析窗口，展示消费数据的可视化分析。
    void onChartButtonClicked();

    /// @brief 处理模型数据改变信号
    /// @param topLeft 改变区域左上角索引
    /// @param bottomRight 改变区域右下角索引
    /// @param roles 改变的角色列表
    ///
    /// 记录修改过的单元格，用于后续的数据验证。
    void onModelDataChanged(const QModelIndex &topLeft,
                            const QModelIndex &bottomRight,
                            const QList<int> &roles);

private:
    Ui::MainWindow *ui;
    const User *currentUser = nullptr;///< 当前登录用户
    QSqlRelationalTableModel *model = nullptr;///< 关系型表格模型

    ChartWindow *chartWindow = nullptr;///< 图表窗口指针

    bool isInternalDataChange = false;///< 内部数据变更标志，防止递归触发
    QSet<QPersistentModelIndex> unValidateDataIndex;///< 待验证的单元格集合
    HighlightDelegate* m_highlightDelegate;///< 高亮委托指针

    /// @brief 在主窗口的状态栏永久显示应用程序版本信息
    ///
    /// 该函数创建一个显示版本号的标签，并将其添加到状态栏的永久区域（右侧）。
    /// 版本信息会一直显示，不会被临时消息覆盖。
    ///
    /// 显示格式为 "version: v[版本号]"，例如 "version: v1.0.0"
    void displayVersion();

    /// @brief 向工具栏添加一个操作按钮（Action）
    ///
    /// 该函数创建一个带有指定文本、图标、快捷键的QAction，并将其添加到工具栏。
    /// 当按钮被点击时，会触发对应的槽函数。
    ///
    /// @param text 按钮显示的文本
    /// @param iconPath 图标的资源路径，如果为空字符串则不设置图标
    /// @param shortcut 快捷键标准键，例如 QKeySequence::New、QKeySequence::Save 等
    /// @param slot 点击按钮时要调用的成员函数指针
    ///
    /// @note
    /// - 图标资源路径使用Qt资源系统格式，例如 ":/icons/icons/add.png"
    /// - 如果不需要快捷键，可以传入 QKeySequence::UnknownKey
    ///
    /// @see QToolBar::addAction()
    void addToolBarAction(const QString &text, const QString &iconPath,
                          const QKeySequence::StandardKey &shortcut,
                          void (MainWindow::*slot)());

    /// @brief 初始化并设置主窗口的工具栏
    ///
    /// 该函数负责创建工具栏的所有操作按钮，包括：
    /// - 设置工具栏为不可移动
    /// - 设置图标大小为24x24像素
    /// - 添加新增、删除、提交、查询、图表分析等操作按钮
    /// - 在相关按钮之间添加分隔符
    ///
    /// 按钮排列顺序（从左到右）：
    /// 1. 新增 - 快捷键 Ctrl+N
    /// 2. 删除 - 快捷键 Delete
    /// 3. 提交 - 快捷键 Ctrl+S
    /// 4. 分隔符
    /// 5. 查询 - 快捷键 Ctrl+F
    /// 6. 分隔符
    /// 7. 图表和AI分析 - 无快捷键
    ///
    /// @note 该函数通常在窗口构造时调用，完成工具栏的一次性初始化
    ///
    /// @see addToolBarAction()
    void setupToolBar();

    /// @brief 为新插入的行设置默认值
    /// @param newRow 新插入的行号
    ///
    /// 设置默认的用户ID、当前日期和金额0。
    void setDefaultValues(const int newRow);

    /// @brief 验证单元格数据的有效性
    /// @param index 需要验证的单元格索引
    /// @return true表示验证通过，false表示验证失败
    ///
    /// 根据字段类型进行不同的验证：
    /// - 日期：非空且格式有效
    /// - 金额：正数且最多2位小数
    /// - 分类：非空
    /// - 描述：长度不超过50个字符
    bool validateCell(const QModelIndex& index);

    /// @brief 高亮或取消高亮指定单元格
    /// @param index 需要操作的单元格索引
    /// @param isError true表示高亮显示（红色背景），false表示取消高亮
    void highlightCell(const QModelIndex &index, bool isError);

    /// @brief 验证分类ID是否存在
    /// @param categoryId 需要验证的分类ID
    /// @return true表示分类存在，false表示分类不存在
    bool validateCategoryExists(int categoryId); // 验证分类是否存在
};
#endif // MAINWINDOW_H
