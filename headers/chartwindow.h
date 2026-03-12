#ifndef CHARTWINDOW_H
#define CHARTWINDOW_H

#include <QMainWindow>
#include <QtCharts>
#include "user.h"

namespace Ui { class ChartWindow; }

class ChartWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit ChartWindow(const User *user, QWidget *parent = nullptr);
    ~ChartWindow();

private:
    void createBarChart();
    void createPieChart();
    void refreshCharts();
    void showAIAnalysis();
    void showAnalysisWithTextBrowser(int year, int month, const QString &analysisResult);
    QString callAIApi(const QString &prompt);

    const User *currentUser = nullptr;
    Ui::ChartWindow *ui;

    QChart *barChart = nullptr;         // 柱状图图表
    QChartView *barChartView = nullptr; // 柱状图视图

    QChart *pieChart = nullptr;         // 饼状图图表
    QChartView *pieChartView = nullptr; // 饼状图视图
};

#endif // CHARTWINDOW_H
