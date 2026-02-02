#include "chartwindow.h"
#include <QBarSet>
#include <QMessageBox>
#include <QSqlQuery>
#include "ui_chartwindow.h"
#include "userdao.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

ChartWindow::ChartWindow(const User *user, QWidget *parent)
    : QMainWindow(parent)
    , currentUser(user)
    , ui(new Ui::ChartWindow) {
    ui->setupUi(this);

    ui->dateEdit->setDate(QDate::currentDate());
    ui->dateEdit->setCalendarPopup(false);

    createBarChart();
    createPieChart();

    ui->categoryBarTab->layout()->addWidget(barChartView);
    ui->categoryPieTab->layout()->addWidget(pieChartView);

    connect(ui->dateEdit, &QDateEdit::dateChanged, this, &ChartWindow::refreshCharts);
    connect(ui->aiBtn, &QPushButton::clicked, this, &ChartWindow::showAIAnalysis);
}

ChartWindow::~ChartWindow() {
    delete ui;

    delete barChart;
    delete barChartView;

    delete pieChart;
    delete pieChartView;
}

/**
 * 创建并显示月度支出分类柱状图
 * 从数据库查询当前用户指定月份的分类支出数据，使用Qt Charts生成柱状图
 */
void ChartWindow::createBarChart() {
    // 获取当前年月
    QDate selectedDate = ui->dateEdit->date();
    int year = selectedDate.year();
    int month = selectedDate.month();

    // 创建日期范围：当月第一天到最后一天
    QDate startDate(year, month, 1);                    // 当月第一天
    QDate endDate = startDate.addMonths(1).addDays(-1); // 当月最后一天

    // 获取当前用户的ID
    int userId = currentUser->getId();

    // 准备SQL查询：按分类统计支出金额
    QSqlQuery query;
    query.prepare("SELECT c.category_name, SUM(e.amount) as total_amount "
                  "FROM Expense e "
                  "JOIN Category c ON e.category_id = c.category_id "
                  "WHERE e.user_id = ? AND e.date >= ? AND e.date <= ? "
                  "GROUP BY c.category_id, c.category_name " // 按分类分组
                  "ORDER BY total_amount DESC");             // 按金额降序排列

    // 绑定查询参数
    query.addBindValue(userId);
    query.addBindValue(startDate.toString("yyyy-MM-dd")); // 开始日期
    query.addBindValue(endDate.toString("yyyy-MM-dd"));   // 结束日期

    // 执行查询
    if (!query.exec()) {
        qWarning() << "Query failed:" << query.lastError().text();
        return;
    }

    // 创建柱状图数据容器
    QBarSet *barSet = new QBarSet("支出金额"); // 数据集合
    QStringList categories;                    // 分类名称列表
    QMap<QString, qreal> categoryAmounts;      // 分类-金额映射表（用于后续计算）

    // 处理查询结果
    while (query.next()) {
        QString categoryName = query.value(0).toString(); // 分类名称
        qreal totalAmount = query.value(1).toReal();      // 分类总金额

        *barSet << totalAmount;                      // 添加数据到柱状图集合
        categories << categoryName;                  // 添加分类名称
        categoryAmounts[categoryName] = totalAmount; // 存储到映射表
    }

    // 设置柱状图样式
    barSet->setColor(QColor(65, 105, 225));      // 设置柱状图颜色（皇家蓝）
    barSet->setBorderColor(QColor(25, 25, 112)); // 设置边框颜色（深蓝）

    // 创建柱状图序列
    QBarSeries *series = new QBarSeries();
    series->append(barSet);          // 添加数据集合
    series->setLabelsVisible(false); // 隐藏柱状图上的数值标签

    // 创建图表对象
    barChart = new QChart();
    barChart->addSeries(series); // 添加数据序列
    barChart->setTitle(QString("%1年%2月支出分类统计 - 柱状图").arg(year).arg(month));
    barChart->setAnimationOptions(QChart::SeriesAnimations); // 启用动画效果
    barChart->legend()->setVisible(true);                    // 显示图例
    barChart->setTheme(QChart::ChartThemeLight);             // 设置浅色主题

    // 创建X轴（分类轴）
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);                 // 设置分类标签
    axisX->setTitleText("支出分类");           // 设置轴标题
    barChart->addAxis(axisX, Qt::AlignBottom); // 添加到图表底部
    series->attachAxis(axisX);                 // 关联数据序列

    // 创建Y轴（数值轴）
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("金额 (元)"); // 设置轴标题
    axisY->setLabelFormat("%.0f");    // 数值显示格式（整数）

    // 设置Y轴标签小字体以适应更多刻度
    axisY->setLabelsFont(QFont("Arial", 6));

    // 动态计算Y轴范围算法：
    // 1. 查找最大金额值
    qreal maxAmount = 0;
    for (const auto &amount : categoryAmounts) {
        if (amount > maxAmount)
            maxAmount = amount;
    }

    // 2. 向上取整到最近的10的整数倍（确保所有数据都能显示）
    qreal roundedMax = ceil(maxAmount / 10.0) * 10;
    if (roundedMax < 10)
        roundedMax = 10; // 确保最小范围为10

    // 3. 设置Y轴范围和刻度
    axisY->setRange(0, roundedMax);             // 设置数值范围
    axisY->setTickCount((roundedMax / 10) + 1); // 每10单位一个刻度

    barChart->addAxis(axisY, Qt::AlignLeft); // 添加到图表左侧
    series->attachAxis(axisY);               // 关联数据序列

    // 创建图表视图
    barChartView = new QChartView(barChart);
    barChartView->setRenderHint(QPainter::Antialiasing); // 启用抗锯齿

    // 添加悬停交互功能：鼠标悬停时显示数值详情
    connect(series,
            &QBarSeries::hovered,
            this,
            [barSet, categories](bool status, int index, QBarSet *set) {
                if (status && index >= 0 && index < categories.size()) {
                    // 获取对应柱状图的数据
                    qreal value = barSet->at(index);
                    // 格式化提示文本：分类名称 + 金额
                    QString tooltip = QString("%1: %2元")
                                          .arg(categories[index])
                                          .arg(value, 0, 'f', 2);

                    // 在鼠标当前位置显示提示
                    QCursor cursor;
                    QToolTip::showText(cursor.pos(), tooltip);
                } else {
                    QToolTip::hideText(); // 隐藏提示
                }
            });
}

void ChartWindow::createPieChart() {
    // 获取当前年月
    QDate selectedDate = ui->dateEdit->date();
    int year = selectedDate.year();
    int month = selectedDate.month();

    // 创建日期范围
    QDate startDate(year, month, 1);
    QDate endDate = startDate.addMonths(1).addDays(-1); // 当月最后一天

    // 获取当前用户的ID
    int userId = currentUser->getId();

    // 查询指定年月的各个分类支出总和
    QSqlQuery query;
    query.prepare("SELECT c.category_name, SUM(e.amount) as total_amount "
                  "FROM Expense e "
                  "JOIN Category c ON e.category_id = c.category_id "
                  "WHERE e.user_id = ? AND e.date >= ? AND e.date <= ? "
                  "GROUP BY c.category_id, c.category_name "
                  "ORDER BY total_amount DESC");

    query.addBindValue(userId);
    query.addBindValue(startDate.toString("yyyy-MM-dd"));
    query.addBindValue(endDate.toString("yyyy-MM-dd"));

    if (!query.exec()) {
        qWarning() << "Query failed:" << query.lastError().text();
        return;
    }

    // 计算总支出
    qreal totalExpense = 0;
    QMap<QString, qreal> categoryAmounts;

    while (query.next()) {
        QString categoryName = query.value(0).toString();
        qreal amount = query.value(1).toReal();
        categoryAmounts[categoryName] = amount;
        totalExpense += amount;
    }

    // 创建饼图序列
    QPieSeries *series = new QPieSeries();
    series->setHoleSize(0.35); // 增加空心大小，减小图形占比
    series->setPieSize(0.7);   // 减小饼图大小

    // 定义颜色
    QList<QColor> colors = {
        QColor(65, 105, 225),  // 皇家蓝
        QColor(220, 20, 60),   // 猩红色
        QColor(46, 139, 87),   // 海绿色
        QColor(255, 140, 0),   // 深橙色
        QColor(147, 112, 219), // 中紫色
        QColor(32, 178, 170),  // 浅海绿色
        QColor(255, 69, 0),    // 红橙色
        QColor(106, 90, 205),  // 板岩蓝
        QColor(205, 133, 63)   // 秘鲁色
    };

    int colorIndex = 0;

    // 添加饼图切片
    for (auto it = categoryAmounts.begin(); it != categoryAmounts.end(); ++it) {
        QString categoryName = it.key();
        qreal amount = it.value();
        qreal percentage = (amount / totalExpense) * 100;

        QString label = QString("%1\n%2元 (%3%)")
                            .arg(categoryName)
                            .arg(amount, 0, 'f', 2)
                            .arg(percentage, 0, 'f', 1);

        QPieSlice *slice = series->append(label, amount);
        slice->setColor(colors[colorIndex % colors.size()]);
        slice->setLabelVisible(true);
        slice->setLabelPosition(QPieSlice::LabelOutside);
        slice->setLabelArmLengthFactor(0.1); // 减小标签连接线长度

        // 设置边框
        slice->setBorderWidth(1);
        slice->setBorderColor(Qt::white);

        colorIndex++;
    }

    // 创建图表
    pieChart = new QChart();
    pieChart->addSeries(series);
    pieChart->setTitle(QString("%1年%2月支出占比分析 - 饼图\n总支出: %3元")
                           .arg(year)
                           .arg(month)
                           .arg(totalExpense, 0, 'f', 2));
    pieChart->legend()->setVisible(true);
    pieChart->legend()->setAlignment(Qt::AlignRight);
    pieChart->setAnimationOptions(QChart::SeriesAnimations);
    pieChart->setTheme(QChart::ChartThemeLight);

    // 设置图表边距，为标签留出更多空间
    pieChart->setMargins(QMargins(10, 10, 10, 10));

    // 设置标题字体
    QFont titleFont;
    titleFont.setBold(true);
    titleFont.setPointSize(10);
    pieChart->setTitleFont(titleFont);

    // 创建图表视图
    pieChartView = new QChartView(pieChart);
    pieChartView->setRenderHint(QPainter::Antialiasing);

    // 添加鼠标悬停效果（只保留突出动画，不移动文字）
    connect(series, &QPieSeries::hovered, this, [](QPieSlice *slice, bool state) {
        if (slice) {
            slice->setExploded(state);
            // 不改变标签可见性，保持原样
        }
    });
}

// 刷新图表函数
void ChartWindow::refreshCharts() {
    // 清除旧图表
    if (barChartView) {
        ui->categoryBarTab->layout()->removeWidget(barChartView);
        delete barChartView;
        barChartView = nullptr;
    }
    if (pieChartView) {
        ui->categoryPieTab->layout()->removeWidget(pieChartView);
        delete pieChartView;
        pieChartView = nullptr;
    }

    // 创建新图表
    createBarChart();
    createPieChart();

    // 添加到布局
    if (barChartView)
        ui->categoryBarTab->layout()->addWidget(barChartView);
    if (pieChartView)
        ui->categoryPieTab->layout()->addWidget(pieChartView);
}

void ChartWindow::showAIAnalysis() {
    // 获取当前年月
    QDate selectedDate = ui->dateEdit->date();
    int year = selectedDate.year();
    int month = selectedDate.month();

    // 创建日期范围
    QDate startDate(year, month, 1);
    QDate endDate = startDate.addMonths(1).addDays(-1); // 当月最后一天

    // 获取当前用户的ID
    int userId = UserDao::getIdByName(currentUser->getName());

    // 使用日期范围查询
    QSqlQuery query;
    query.prepare("SELECT c.category_name, SUM(e.amount) as total_amount "
                  "FROM Expense e "
                  "JOIN Category c ON e.category_id = c.category_id "
                  "WHERE e.user_id = ? AND e.date >= ? AND e.date <= ? "
                  "GROUP BY c.category_id, c.category_name "
                  "ORDER BY total_amount DESC");

    query.addBindValue(userId);
    query.addBindValue(startDate.toString("yyyy-MM-dd"));
    query.addBindValue(endDate.toString("yyyy-MM-dd"));

    if (!query.exec()) {
        qWarning() << "Query failed:" << query.lastError().text();
        QMessageBox::warning(this, "错误", "数据查询失败：" + query.lastError().text());
        return;
    }

    // 收集分类和金额数据
    QMap<QString, qreal> categoryAmounts;
    qreal totalAmount = 0;

    while (query.next()) {
        QString categoryName = query.value(0).toString();
        qreal amount = query.value(1).toReal();
        categoryAmounts[categoryName] = amount;
        totalAmount += amount;
    }

    if (categoryAmounts.isEmpty()) {
        QMessageBox::information(this,
                                 "分析结果",
                                 selectedDate.toString("yyyy年MM月") + "没有支出数据");
        return;
    }

    // 构建发送给AI的提示信息
    QString prompt = QString("你是一名专业的财务顾问，请分析以下%1年%"
                             "2月的个人支出数据，并提供专业的财务建议。("
                             "全部回复使用Markdown格式)\n")
                         .arg(year)
                         .arg(month);

    prompt += QString("总支出：%1元\n").arg(totalAmount, 0, 'f', 2);
    prompt += "分类支出详情：\n";

    for (auto it = categoryAmounts.constBegin(); it != categoryAmounts.constEnd(); ++it) {
        double percentage = (it.value() / totalAmount) * 100;
        prompt += QString("%1: %2元 (%3%)\n")
                      .arg(it.key())
                      .arg(it.value(), 0, 'f', 2)
                      .arg(percentage, 0, 'f', 1);
    }

    // 添加分析指令和要求
    prompt += QString("【分析要求】\n"
                      "请从以下角度进行分析：\n"
                      "1. 支出结构合理性：分析各分类占比是否合理，是否存在过度消费的领域\n"
                      "2. 消费习惯评估：识别可能的消费模式和行为特点\n"
                      "3. 优化建议：提供具体的节省建议和预算调整方案\n"
                      "4. 风险提示：指出潜在的财务风险\n\n"
                      "【输出格式要求】\n"
                      "1. 语言简洁明了，使用中文回复\n"
                      "2. 字数控制在200-300字之间\n"
                      "3. 结构清晰，包含：整体评价、主要问题、具体建议\n"
                      "4. 建议要具体可行，避免泛泛而谈\n"
                      "5. 使用专业的财务术语，但解释要通俗易懂\n\n"
                      "请开始你的分析：");

    // 显示等待对话框
    QMessageBox::information(this, "提示", "正在请求AI分析，请稍候...");

    // 调用AI分析
    QString analysisResult = callAIApi(prompt);

    // 使用QTextBrowser显示分析结果
    showAnalysisWithTextBrowser(year, month, analysisResult);
}

// 新增的显示函数
void ChartWindow::showAnalysisWithTextBrowser(int year, int month, const QString &analysisResult) {
    // 创建自定义对话框
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(
        QString("%1年%2月支出分析 - AI智能分析报告").arg(year).arg(month, 2, 10, QLatin1Char('0')));
    dialog->setMinimumSize(700, 500);
    dialog->setMaximumSize(1000, 700);

    // 设置布局
    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);

    // 创建QTextBrowser
    QTextBrowser *textBrowser = new QTextBrowser(dialog);
    textBrowser->setOpenExternalLinks(true);

    // 直接设置AI生成的Markdown内容
    textBrowser->setMarkdown(analysisResult);
    textBrowser->setStyleSheet(R"(
        QTextBrowser {
            background-color: #f8f9fa;
            border: 1px solid #dee2e6;
            border-radius: 8px;
            padding: 15px;
            font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
            font-size: 11pt;
        }
        QTextBrowser a {
            color: #007bff;
            text-decoration: none;
        }
        QTextBrowser a:hover {
            text-decoration: underline;
        }
    )");

    // 创建关闭按钮
    QPushButton *closeButton = new QPushButton("关闭", dialog);
    closeButton->setStyleSheet(R"(
        QPushButton {
            background-color: #007bff;
            color: white;
            border: none;
            padding: 8px 20px;
            border-radius: 6px;
            font-weight: bold;
            margin: 10px;
        }
        QPushButton:hover {
            background-color: #0056b3;
        }
    )");

    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

    // 添加到布局
    mainLayout->addWidget(textBrowser);
    mainLayout->addWidget(closeButton, 0, Qt::AlignRight);

    dialog->setLayout(mainLayout);

    // 显示对话框
    dialog->exec();
    dialog->deleteLater();
}

QString ChartWindow::callAIApi(const QString &prompt) {
    QNetworkAccessManager manager;
    QEventLoop eventLoop;
    QTimer timer;

    // 设置超时（60秒）
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    QObject::connect(&manager, &QNetworkAccessManager::finished, &eventLoop, &QEventLoop::quit);

    // API配置 - 使用OpenAI兼容格式
    const QString apiUrl = "https://ai.gitee.com/v1/chat/completions";
    const QString apiKey = "KQMAPVFJQ7ICYZCDAYSDJBFBXYWYJ5UNC2NR7L1B";
    const QString model = "glm-4-9b-chat"; // 使用OpenAI兼容的模型名称

    // 构建请求JSON - 使用标准的OpenAI格式
    QJsonObject message;
    message["role"] = "user";
    message["content"] = prompt;

    QJsonArray messages;
    messages.append(message);

    QJsonObject requestBody;
    requestBody["model"] = model;
    requestBody["messages"] = messages;
    requestBody["stream"] = false; // 禁用流式输出
    requestBody["max_tokens"] = 1000;
    requestBody["temperature"] = 0.7;
    requestBody["top_p"] = 0.9;

    QJsonDocument jsonDoc(requestBody);
    QByteArray data = jsonDoc.toJson();

    // 构建网络请求 - 使用OpenAI标准格式
    QNetworkRequest request((QUrl(apiUrl)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 使用Bearer Token认证（OpenAI标准格式）
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());

    // 可选：添加其他可能的头信息
    request.setRawHeader("Accept", "application/json");

    // 发送请求
    QNetworkReply *reply = manager.post(request, data);
    timer.start(60000); // 60秒超时

    eventLoop.exec(); // 等待请求完成

    // 处理超时
    if (!timer.isActive()) {
        reply->abort();
        return "请求超时，请检查网络连接后重试。";
    }

    // 检查网络错误
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = QString("网络错误: %1").arg(reply->errorString());
        qDebug() << "网络错误详情:" << errorMsg;
        qDebug() << "HTTP状态码:"
                 << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();
        return errorMsg;
    }

    // 获取响应状态码
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "HTTP状态码:" << statusCode;

    // 解析响应
    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);
    if (jsonResponse.isNull()) {
        qDebug() << "响应不是有效的JSON格式";
        return "API响应解析失败 - 不是有效的JSON格式";
    }

    QJsonObject root = jsonResponse.object();

    // 检查API错误 - OpenAI标准错误格式
    if (root.contains("error")) {
        QJsonObject errorObj = root["error"].toObject();
        QString errorMsg = "未知错误";

        if (errorObj.contains("message")) {
            errorMsg = errorObj["message"].toString();
        } else if (errorObj.contains("code")) {
            errorMsg = QString("错误代码: %1").arg(errorObj["code"].toString());
        }

        return QString("API错误: %1").arg(errorMsg);
    }

    // 检查HTTP错误状态
    if (statusCode >= 400) {
        QString errorMsg = root.contains("message") ? root["message"].toString()
                                                    : QString("HTTP错误: %1").arg(statusCode);
        return errorMsg;
    }

    // 提取AI回复内容 - 使用OpenAI标准响应格式
    QString result;

    // OpenAI标准格式：choices[0].message.content
    if (root.contains("choices")) {
        QJsonArray choices = root["choices"].toArray();
        if (!choices.isEmpty()) {
            QJsonObject firstChoice = choices[0].toObject();

            if (firstChoice.contains("message")) {
                QJsonObject messageObj = firstChoice["message"].toObject();
                if (messageObj.contains("content")) {
                    result = messageObj["content"].toString().trimmed();
                }
            }
        }
    }

    if (!result.isEmpty()) {
        return result;
    }

    // 备用解析方式
    if (root.contains("content")) {
        result = root["content"].toString().trimmed();
    } else if (root.contains("result")) {
        result = root["result"].toString().trimmed();
    }

    if (!result.isEmpty()) {
        return result;
    }

    // 如果所有方式都失败，返回原始响应用于调试
    return QString("无法解析AI响应内容。请检查API响应格式。原始响应:\n%1")
        .arg(QString::fromUtf8(responseData));
}
