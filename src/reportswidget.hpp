#pragma once

#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QWidget>
#include <QtCharts/QChartView>
#include "models.hpp"

class DashboardTab : public QWidget {
    Q_OBJECT
  public:
    explicit DashboardTab(QWidget* parent = nullptr);
    void refresh();

  private:
    QLabel* m_todayLabel;
    QLabel* m_weekLabel;
    QLabel* m_monthLabel;
    QLabel* m_yearLabel;
    QTableWidget* m_dailyTable;
    QTableWidget* m_monthlyTable;
    QTableWidget* m_annualTable;
    QChartView* m_weeklyChart;
    QChartView* m_monthlyChart;

    void setupUi();
    void loadData();
    void updateSummaryCard(QLabel* label, double value);
};

class SalesReportTab : public QWidget {
    Q_OBJECT
  public:
    explicit SalesReportTab(QWidget* parent = nullptr);
    void refresh();
  private slots:
    void onGenerate();

  private:
    QComboBox* m_periodCombo;
    QDateEdit* m_dateFrom;
    QDateEdit* m_dateTo;
    QTableWidget* m_table;
    QLabel* m_totalLabel;
    void setupUi();
};

class StockCardTab : public QWidget {
    Q_OBJECT
  public:
    explicit StockCardTab(QWidget* parent = nullptr);
    void refresh();
  private slots:
    void onGenerate();

  private:
    QDateEdit* m_dateFrom;
    QDateEdit* m_dateTo;
    QTableWidget* m_table;
    void setupUi();
};

class ReportsWidget : public QWidget {
    Q_OBJECT
  public:
    explicit ReportsWidget(const User& user, QWidget* parent = nullptr);
    void refresh();

  private:
    User m_currentUser;
    QTabWidget* m_tabs;
    DashboardTab* m_dashTab;
    SalesReportTab* m_salesTab;
    StockCardTab* m_stockTab;
    void setupUi();
};
