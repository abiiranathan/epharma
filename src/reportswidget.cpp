#include "reportswidget.hpp"
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPrintDialog>
#include <QPrinter>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPieSeries>
#include <QtCharts/QValueAxis>
#include "database.hpp"

// =================== Helpers ===================

static QString fmtCurrency(double v) { return QString("UGX %L1").arg(v, 0, 'f', 2); }

static QWidget* makeStatCard(const QString& title, QLabel*& valueLabel, const QString& bg,
                             const QString& fg = "#1e3a5f") {
    auto* card = new QWidget;
    card->setObjectName("statsCard");
    card->setStyleSheet(QString("QWidget#statsCard { background-color: %1; border-radius: 10px; "
                                "border: 1px solid #e0e6ed; padding: 16px; }")
                            .arg(bg));
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(6);

    auto* titleLbl = new QLabel(title);
    titleLbl->setObjectName("cardTitle");
    titleLbl->setStyleSheet(
        "font-size: 11px; font-weight: 600; color: #6b7a99; "
        "text-transform: uppercase; letter-spacing: 0.5px;");

    valueLabel = new QLabel("UGX 0.00");
    valueLabel->setObjectName("cardValue");
    valueLabel->setStyleSheet(QString("font-size: 22px; font-weight: 700; color: %1;").arg(fg));

    layout->addWidget(titleLbl);
    layout->addWidget(valueLabel);
    return card;
}

// =================== Sales Detail Dialog ===================
static void showSalesDetailDialog(QWidget* parent, const QString& title, const QList<ProductSale>& sales) {
    auto* dlg = new QDialog(parent, Qt::Dialog);
    dlg->setWindowTitle(title);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->resize(1200, 600);

    auto* root = new QVBoxLayout(dlg);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    // Title bar inside dialog
    auto* heading = new QLabel(title);
    heading->setStyleSheet("font-size: 15px; font-weight: 700; color: #1e3a5f;");
    root->addWidget(heading);

    // Summary strip
    double totalIncome = 0, totalProfit = 0;
    int totalQty = 0;
    for (const auto& s : sales) {
        totalIncome += s.income;
        totalProfit += s.profit;
        totalQty += s.quantitySold;
    }

    auto* summaryRow = new QHBoxLayout;
    auto makeSummaryCard = [&](const QString& lbl, const QString& val, const QString& color) {
        auto* w = new QWidget;
        w->setStyleSheet(QString("background:%1;border-radius:6px;padding:8px 14px;").arg(color));
        auto* lay = new QVBoxLayout(w);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(2);
        auto* l = new QLabel(lbl);
        l->setStyleSheet("font-size:10px;font-weight:600;color:#555;");
        auto* v = new QLabel(val);
        v->setStyleSheet("font-size:14px;font-weight:700;color:#1e3a5f;");
        lay->addWidget(l);
        lay->addWidget(v);
        return w;
    };

    summaryRow->addWidget(makeSummaryCard("TOTAL INCOME", fmtCurrency(totalIncome), "#ebf8ff"));
    summaryRow->addSpacing(8);
    summaryRow->addWidget(makeSummaryCard("TOTAL PROFIT", fmtCurrency(totalProfit), "#f0fff4"));
    summaryRow->addSpacing(8);
    summaryRow->addWidget(makeSummaryCard("UNITS SOLD", QString::number(totalQty), "#fffbeb"));
    summaryRow->addSpacing(8);
    summaryRow->addWidget(makeSummaryCard("PRODUCT LINES", QString::number(sales.size()), "#faf5ff"));
    summaryRow->addStretch();
    root->addLayout(summaryRow);

    // Table
    auto* tbl = new QTableWidget;
    tbl->setColumnCount(6);
    tbl->setHorizontalHeaderLabels({"Product", "Cost Price", "Selling Price", "Qty Sold", "Income", "Profit"});
    tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tbl->setAlternatingRowColors(true);
    tbl->setSelectionBehavior(QAbstractItemView::SelectRows);
    tbl->verticalHeader()->setVisible(false);
    tbl->setShowGrid(false);
    tbl->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    tbl->setColumnWidth(1, 110);
    tbl->setColumnWidth(2, 120);
    tbl->setColumnWidth(3, 90);
    tbl->setColumnWidth(4, 130);
    tbl->setColumnWidth(5, 130);

    tbl->setRowCount(static_cast<int>(sales.size()));
    for (int i = 0; i < sales.size(); ++i) {
        const auto& s = sales[i];

        tbl->setItem(i, 0, new QTableWidgetItem(s.productName));

        auto makeRight = [](const QString& text, const QColor& fg = QColor()) -> QTableWidgetItem* {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            if (fg.isValid()) {
                item->setForeground(fg);
            }
            return item;
        };

        tbl->setItem(i, 1, makeRight(fmtCurrency(s.costPrice)));
        tbl->setItem(i, 2, makeRight(fmtCurrency(s.sellingPrice)));

        auto* qtyItem = new QTableWidgetItem(QString::number(s.quantitySold));
        qtyItem->setTextAlignment(Qt::AlignCenter);
        tbl->setItem(i, 3, qtyItem);

        tbl->setItem(i, 4, makeRight(fmtCurrency(s.income), QColor("#2b6cb0")));

        auto* profItem = makeRight(fmtCurrency(s.profit), s.profit >= 0 ? QColor("#2f855a") : QColor("#e53e3e"));
        profItem->setFont(QFont("", -1, QFont::Bold));
        tbl->setItem(i, 5, profItem);
    }

    if (sales.isEmpty()) {
        tbl->setRowCount(1);
        auto* empty = new QTableWidgetItem("No sales data for this period.");
        empty->setTextAlignment(Qt::AlignCenter);
        empty->setForeground(QColor("#a0aec0"));
        tbl->setItem(0, 0, empty);
        tbl->setSpan(0, 0, 1, 6);
    }

    root->addWidget(tbl);

    // Close button
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Close);
    QAbstractEventDispatcher::connect(btnBox, &QDialogButtonBox::rejected, dlg, &QDialog::accept);
    root->addWidget(btnBox);

    dlg->exec();
}

// =================== DashboardTab ===================

DashboardTab::DashboardTab(QWidget* parent) : QWidget(parent) { setupUi(); }

void DashboardTab::setupUi() {
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* content = new QWidget;
    auto* root = new QVBoxLayout(content);
    root->setContentsMargins(16, 16, 16, 24);
    root->setSpacing(16);

    // Stats row
    auto* statsRow = new QHBoxLayout;
    statsRow->setSpacing(12);
    statsRow->addWidget(makeStatCard("Today's Sales", m_todayLabel, "#ebf8ff", "#2b6cb0"));
    statsRow->addWidget(makeStatCard("This Week", m_weekLabel, "#f0fff4", "#276749"));
    statsRow->addWidget(makeStatCard("This Month", m_monthLabel, "#fffbeb", "#b7791f"));
    statsRow->addWidget(makeStatCard("This Year", m_yearLabel, "#faf5ff", "#553c9a"));
    root->addLayout(statsRow);

    // Charts
    auto* chartsRow = new QHBoxLayout;
    chartsRow->setSpacing(12);

    m_weeklyChart = new QChartView;
    m_weeklyChart->setMinimumHeight(240);
    m_weeklyChart->setRenderHint(QPainter::Antialiasing);

    m_monthlyChart = new QChartView;
    m_monthlyChart->setMinimumHeight(240);
    m_monthlyChart->setRenderHint(QPainter::Antialiasing);

    chartsRow->addWidget(m_weeklyChart);
    chartsRow->addWidget(m_monthlyChart);
    root->addLayout(chartsRow);

    // Tables row
    auto* tablesRow = new QHBoxLayout;
    tablesRow->setSpacing(12);

    auto makeTableGroup = [&](const QString& title, QTableWidget*& tbl, const QStringList& headers) -> QGroupBox* {
        auto* grp = new QGroupBox(title);
        auto* lay = new QVBoxLayout(grp);
        tbl = new QTableWidget;
        tbl->setColumnCount(static_cast<int>(headers.size()));
        tbl->setHorizontalHeaderLabels(headers);
        tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
        tbl->setAlternatingRowColors(true);
        tbl->verticalHeader()->setVisible(false);
        tbl->setShowGrid(false);
        tbl->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        tbl->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);

        tbl->setColumnWidth(2, 120);
        tbl->horizontalHeader()->setStretchLastSection(false);
        tbl->setMaximumHeight(260);
        tbl->setStyleSheet("QTableWidget { padding: 8px; }");

        lay->addWidget(tbl);
        return grp;
    };

    tablesRow->addWidget(makeTableGroup("Daily Sales (Last 14 Days)", m_dailyTable, {"Date", "Total Income", "View"}));
    tablesRow->addWidget(makeTableGroup("Monthly Sales", m_monthlyTable, {"Month", "Total Income", "View"}));
    tablesRow->addWidget(makeTableGroup("Annual Sales", m_annualTable, {"Year", "Total Income", "View"}));

    root->addLayout(tablesRow);

    scroll->setWidget(content);
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(scroll);
}

void DashboardTab::loadData() {  // NOLINT
    QDate today = QDate::currentDate();
    QDate weekStart = today.addDays(-today.dayOfWeek() + 1);
    QDate monthStart = QDate(today.year(), today.month(), 1);
    QDate yearStart = QDate(today.year(), 1, 1);

    QList<SalesReport> daily = Database::instance().getDailySalesReports();
    QList<MonthlySalesReport> monthly = Database::instance().getMonthlySalesReports();
    QList<AnnualSalesReport> annual = Database::instance().getAnnualSalesReports();

    // Aggregate
    double incToday = 0, incWeek = 0, incMonth = 0, incYear = 0;
    for (const auto& r : daily) {
        QDate d = r.transactionDate;
        if (d == today) {
            incToday += r.totalIncome;
        }
        if (d >= weekStart) {
            incWeek += r.totalIncome;
        }
        if (d >= monthStart) {
            incMonth += r.totalIncome;
        }
        if (d >= yearStart) {
            incYear += r.totalIncome;
        }
    }

    m_todayLabel->setText(fmtCurrency(incToday));
    m_weekLabel->setText(fmtCurrency(incWeek));
    m_monthLabel->setText(fmtCurrency(incMonth));
    m_yearLabel->setText(fmtCurrency(incYear));

    // Weekly chart (Mon-Sun)
    QList<QString> dayLabels = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    QList<double> dayValues(7, 0.0);
    for (const auto& r : daily) {
        if (r.transactionDate >= weekStart && r.transactionDate <= today) {
            int idx = r.transactionDate.dayOfWeek() - 1;
            if (idx >= 0 && idx < 7) {
                dayValues[idx] += r.totalIncome;
            }
        }
    }

    {
        auto* chart = new QChart;
        chart->setTitle("This Week's Sales");
        chart->setAnimationOptions(QChart::SeriesAnimations);
        chart->setBackgroundBrush(Qt::white);
        chart->setTitleFont(QFont("Segoe UI", 10, QFont::Bold));
        auto* set = new QBarSet("Income");
        set->setColor(QColor("#3a7bd5"));
        for (double v : dayValues) {
            *set << v;
        }
        auto* series = new QBarSeries;
        series->append(set);
        chart->addSeries(series);
        auto* axisX = new QBarCategoryAxis;
        axisX->append(dayLabels);
        axisX->setLabelsFont(QFont("Segoe UI", 8));
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);
        auto* axisY = new QValueAxis;
        axisY->setLabelFormat("%.0f");
        axisY->setLabelsFont(QFont("Segoe UI", 8));
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
        chart->legend()->setVisible(false);
        m_weeklyChart->setChart(chart);
        m_weeklyChart->setRenderHint(QPainter::Antialiasing);
    }

    // Monthly chart (Jan-Dec current year)
    QList<QString> monthLabels = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    QList<double> monthValues(12, 0.0);
    for (const auto& r : monthly) {
        if (r.month.year() == today.year()) {
            int idx = r.month.month() - 1;
            if (idx >= 0 && idx < 12) {
                monthValues[idx] += r.totalIncome;
            }
        }
    }

    {
        auto* chart = new QChart;
        chart->setTitle(QString("Monthly Sales (%1)").arg(today.year()));
        chart->setAnimationOptions(QChart::SeriesAnimations);
        chart->setBackgroundBrush(Qt::white);
        chart->setTitleFont(QFont("Segoe UI", 10, QFont::Bold));
        auto* set = new QBarSet("Income");
        set->setColor(QColor("#2f855a"));
        for (double v : monthValues) {
            *set << v;
        }
        auto* series = new QBarSeries;
        series->append(set);
        chart->addSeries(series);
        auto* axisX = new QBarCategoryAxis;
        axisX->append(monthLabels);
        axisX->setLabelsFont(QFont("Segoe UI", 8));
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);
        auto* axisY = new QValueAxis;
        axisY->setLabelFormat("%.0f");
        axisY->setLabelsFont(QFont("Segoe UI", 8));
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
        chart->legend()->setVisible(false);
        m_monthlyChart->setChart(chart);
        m_monthlyChart->setRenderHint(QPainter::Antialiasing);
    }

    // ---- Daily table ----
    m_dailyTable->setRowCount(0);
    int shown = 0;
    for (const auto& r : daily) {
        if (shown >= 14) {
            break;
        }
        int row = m_dailyTable->rowCount();
        m_dailyTable->insertRow(row);
        m_dailyTable->setItem(row, 0, new QTableWidgetItem(r.transactionDate.toString("ddd, dd MMM yyyy")));

        auto* incItem = new QTableWidgetItem(fmtCurrency(r.totalIncome));
        incItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        incItem->setForeground(QColor("#2f855a"));
        m_dailyTable->setItem(row, 1, incItem);

        auto* viewBtn = new QPushButton("View");
        viewBtn->setFixedHeight(22);
        viewBtn->setStyleSheet(
            "QPushButton { background-color: #3a7bd5; color: white; border: none; "
            "border-radius: 3px; font-size: 11px; padding: 0 6px; }");
        QDate d = r.transactionDate;
        connect(viewBtn, &QPushButton::clicked, this, [d, this] {
            auto sales = Database::instance().getDailyProductSales(d);
            showSalesDetailDialog(this, QString("Daily Sales — %1").arg(d.toString("dddd, dd MMM yyyy")), sales);
        });
        m_dailyTable->setCellWidget(row, 2, viewBtn);
        shown++;
    }
    m_dailyTable->setColumnWidth(0, 160);
    m_dailyTable->setColumnWidth(1, 110);

    // ---- Monthly table ----
    m_monthlyTable->setRowCount(0);
    for (const auto& r : monthly) {
        int row = m_monthlyTable->rowCount();
        m_monthlyTable->insertRow(row);
        m_monthlyTable->setItem(row, 0, new QTableWidgetItem(r.month.toString("MMMM yyyy")));

        auto* incItem = new QTableWidgetItem(fmtCurrency(r.totalIncome));
        incItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        incItem->setForeground(QColor("#2f855a"));
        m_monthlyTable->setItem(row, 1, incItem);

        auto* viewBtn = new QPushButton("View");
        viewBtn->setFixedHeight(22);
        viewBtn->setStyleSheet(
            "QPushButton { background-color: #3a7bd5; color: white; border: none; "
            "border-radius: 3px; font-size: 11px; padding: 0 6px; }");
        int yr = r.month.year();
        int mo = r.month.month();
        connect(viewBtn, &QPushButton::clicked, this, [this, yr, mo] {
            auto sales = Database::instance().getMonthlyProductSales(yr, mo);
            showSalesDetailDialog(this, QString("Monthly Sales — %1/%2").arg(mo, 2, 10, QChar('0')).arg(yr), sales);
        });
        m_monthlyTable->setCellWidget(row, 2, viewBtn);
    }
    m_monthlyTable->setColumnWidth(0, 120);
    m_monthlyTable->setColumnWidth(1, 110);

    // ---- Annual table ----
    m_annualTable->setRowCount(0);
    for (const auto& r : annual) {
        int row = m_annualTable->rowCount();
        m_annualTable->insertRow(row);
        m_annualTable->setItem(row, 0, new QTableWidgetItem(QString::number(r.year.year())));

        auto* incItem = new QTableWidgetItem(fmtCurrency(r.totalIncome));
        incItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        incItem->setForeground(QColor("#2f855a"));
        m_annualTable->setItem(row, 1, incItem);

        auto* viewBtn = new QPushButton("View");
        viewBtn->setFixedHeight(22);
        viewBtn->setStyleSheet(
            "QPushButton { background-color: #3a7bd5; color: white; border: none; "
            "border-radius: 3px; font-size: 11px; padding: 0 6px; }");
        int yr = r.year.year();
        connect(viewBtn, &QPushButton::clicked, this, [this, yr] {
            auto sales = Database::instance().getAnnualProductSales(yr);
            showSalesDetailDialog(this, QString("Annual Sales — %1").arg(yr), sales);
        });
        m_annualTable->setCellWidget(row, 2, viewBtn);
    }
    m_annualTable->setColumnWidth(0, 70);
    m_annualTable->setColumnWidth(1, 110);
}

void DashboardTab::refresh() { loadData(); }

// =================== SalesReportTab ===================

SalesReportTab::SalesReportTab(QWidget* parent) : QWidget(parent) { setupUi(); }

void SalesReportTab::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto* title = new QLabel("Detailed Sales Report");
    title->setStyleSheet("font-size: 15px; font-weight: 700; color: #1e3a5f;");
    root->addWidget(title);

    // Controls
    auto* ctrlRow = new QHBoxLayout;
    ctrlRow->setSpacing(12);

    auto* periodLabel = new QLabel("Period:");
    periodLabel->setStyleSheet("font-weight: 600; color: #4a5568;");
    m_periodCombo = new QComboBox;
    m_periodCombo->addItems({"Daily", "Monthly", "Annual"});
    m_periodCombo->setFixedWidth(120);

    auto* fromLabel = new QLabel("From:");
    fromLabel->setStyleSheet("font-weight: 600; color: #4a5568;");
    m_dateFrom = new QDateEdit(QDate::currentDate().addMonths(-1));
    m_dateFrom->setCalendarPopup(true);
    m_dateFrom->setDisplayFormat("yyyy-MM-dd");

    auto* toLabel = new QLabel("To:");
    toLabel->setStyleSheet("font-weight: 600; color: #4a5568;");
    m_dateTo = new QDateEdit(QDate::currentDate());
    m_dateTo->setCalendarPopup(true);
    m_dateTo->setDisplayFormat("yyyy-MM-dd");

    auto* genBtn = new QPushButton("Generate Report");
    genBtn->setObjectName("successBtn");
    genBtn->setFixedHeight(34);

    ctrlRow->addWidget(periodLabel);
    ctrlRow->addWidget(m_periodCombo);
    ctrlRow->addWidget(fromLabel);
    ctrlRow->addWidget(m_dateFrom);
    ctrlRow->addWidget(toLabel);
    ctrlRow->addWidget(m_dateTo);
    ctrlRow->addStretch();
    ctrlRow->addWidget(genBtn);
    root->addLayout(ctrlRow);

    m_totalLabel = new QLabel("Total: UGX 0.00");
    m_totalLabel->setStyleSheet(
        "font-size: 18px; font-weight: 700; color: #2f855a; "
        "background: #f0fff4; border-radius: 6px; padding: 8px 16px;");
    root->addWidget(m_totalLabel);

    m_table = new QTableWidget;
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({"Date", "Product", "Qty Sold", "Cost", "Selling Price", "Income", "Profit"});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setColumnWidth(0, 120);
    m_table->setColumnWidth(2, 100);
    m_table->setColumnWidth(3, 120);
    m_table->setColumnWidth(4, 120);
    m_table->setColumnWidth(5, 130);
    m_table->setColumnWidth(6, 130);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    root->addWidget(m_table);

    connect(genBtn, &QPushButton::clicked, this, &SalesReportTab::onGenerate);
}

void SalesReportTab::onGenerate() {
    QList<ProductSale> sales;
    QString period = m_periodCombo->currentText();
    QDate from = m_dateFrom->date();
    QDate to = m_dateTo->date();

    if (period == "Daily") {
        QDate d = from;
        while (d <= to) {
            sales += Database::instance().getDailyProductSales(d);
            d = d.addDays(1);
        }
    } else if (period == "Monthly") {
        for (int y = from.year(); y <= to.year(); ++y) {
            int m1 = (y == from.year()) ? from.month() : 1;
            int m2 = (y == to.year()) ? to.month() : 12;
            for (int m = m1; m <= m2; ++m) {
                sales += Database::instance().getMonthlyProductSales(y, m);
            }
        }
    } else {
        for (int y = from.year(); y <= to.year(); ++y) {
            sales += Database::instance().getAnnualProductSales(y);
        }
    }

    m_table->setRowCount(0);
    m_table->setRowCount(static_cast<int>(sales.size()));
    double totalProfit = 0, totalIncome = 0;

    for (int i = 0; i < sales.size(); ++i) {
        const auto& s = sales[i];
        m_table->setItem(i, 0, new QTableWidgetItem(s.transactionDate.toString("dd MMM yyyy")));
        m_table->setItem(i, 1, new QTableWidgetItem(s.productName));

        auto* qtyItem = new QTableWidgetItem(QString::number(s.quantitySold));
        qtyItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 2, qtyItem);

        auto makeRight = [](double v) -> QTableWidgetItem* {
            auto* item = new QTableWidgetItem(QString("UGX %1").arg(v, 0, 'f', 2));
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            return item;
        };

        m_table->setItem(i, 3, makeRight(s.costPrice));
        m_table->setItem(i, 4, makeRight(s.sellingPrice));

        auto* incItem = makeRight(s.income);
        incItem->setForeground(QColor("#2b6cb0"));
        m_table->setItem(i, 5, incItem);

        auto* profItem = makeRight(s.profit);
        profItem->setForeground(s.profit >= 0 ? QColor("#2f855a") : QColor("#e53e3e"));
        profItem->setFont(QFont("", -1, QFont::Bold));
        m_table->setItem(i, 6, profItem);

        totalIncome += s.income;
        totalProfit += s.profit;
    }

    m_totalLabel->setText(QString("Income: %1   |   Profit: %2   |   Items: %3")
                              .arg(fmtCurrency(totalIncome))
                              .arg(fmtCurrency(totalProfit))
                              .arg(sales.size()));
}

void SalesReportTab::refresh() { onGenerate(); }

// =================== StockCardTab ===================

StockCardTab::StockCardTab(QWidget* parent) : QWidget(parent) { setupUi(); }

void StockCardTab::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto* title = new QLabel("Stock Card");
    title->setStyleSheet("font-size: 15px; font-weight: 700; color: #1e3a5f;");
    root->addWidget(title);

    auto* desc = new QLabel("Daily opening balance, stock in, stock out, and closing balance.");
    desc->setStyleSheet("color: #718096; font-size: 12px;");
    root->addWidget(desc);

    auto* ctrlRow = new QHBoxLayout;
    ctrlRow->setSpacing(12);

    auto* fromLabel = new QLabel("From:");
    fromLabel->setStyleSheet("font-weight: 600; color: #4a5568;");
    m_dateFrom = new QDateEdit(QDate::currentDate().addDays(-7));
    m_dateFrom->setCalendarPopup(true);
    m_dateFrom->setDisplayFormat("yyyy-MM-dd");

    auto* toLabel = new QLabel("To:");
    toLabel->setStyleSheet("font-weight: 600; color: #4a5568;");
    m_dateTo = new QDateEdit(QDate::currentDate());
    m_dateTo->setCalendarPopup(true);
    m_dateTo->setDisplayFormat("yyyy-MM-dd");

    auto* genBtn = new QPushButton("Generate Stock Card");
    genBtn->setObjectName("successBtn");
    genBtn->setFixedHeight(34);

    ctrlRow->addWidget(fromLabel);
    ctrlRow->addWidget(m_dateFrom);
    ctrlRow->addWidget(toLabel);
    ctrlRow->addWidget(m_dateTo);
    ctrlRow->addStretch();
    ctrlRow->addWidget(genBtn);
    root->addLayout(ctrlRow);

    m_table = new QTableWidget;
    m_table->setColumnCount(9);
    m_table->setHorizontalHeaderLabels(
        {"Date", "Product", "Brand", "Opening", "In", "Out", "Reversal", "Closing", "Status"});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setColumnWidth(0, 110);
    m_table->setColumnWidth(2, 120);
    m_table->setColumnWidth(3, 70);
    m_table->setColumnWidth(4, 60);
    m_table->setColumnWidth(5, 60);
    m_table->setColumnWidth(6, 70);  // Reversal
    m_table->setColumnWidth(7, 70);  // Closing
    m_table->setColumnWidth(8, 80);  // Status
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    root->addWidget(m_table);

    connect(genBtn, &QPushButton::clicked, this, &StockCardTab::onGenerate);
}

void StockCardTab::onGenerate() {
    QList<StockCard> cards = Database::instance().getStockCard(m_dateFrom->date(), m_dateTo->date());
    m_table->setRowCount(0);
    m_table->setRowCount(static_cast<int>(cards.size()));

    for (int i = 0; i < cards.size(); ++i) {
        const auto& sc = cards[i];
        m_table->setItem(i, 0, new QTableWidgetItem(sc.date.toString("dd MMM yyyy")));
        m_table->setItem(i, 1, new QTableWidgetItem(sc.genericName));
        m_table->setItem(i, 2, new QTableWidgetItem(sc.brandName));

        auto center = [](int v, const QString& color = "") -> QTableWidgetItem* {
            auto* item = new QTableWidgetItem(QString::number(v));
            item->setTextAlignment(Qt::AlignCenter);
            if (!color.isEmpty()) {
                item->setForeground(QColor(color));
            }
            return item;
        };

        m_table->setItem(i, 3, center(sc.openingQuantity));

        auto* inItem = center(sc.quantityIn, sc.quantityIn > 0 ? "#2f855a" : "");
        m_table->setItem(i, 4, inItem);

        m_table->setItem(i, 5, center(sc.quantityOut, "#e53e3e"));

        auto* revItem = center(sc.quantityReversal, sc.quantityReversal > 0 ? "#2b6cb0" : "");
        m_table->setItem(i, 6, revItem);

        auto* closItem = center(sc.closingQuantity);
        if (sc.closingQuantity == 0) {
            closItem->setForeground(QColor("#e53e3e"));
        } else if (sc.closingQuantity < 10) {
            closItem->setForeground(QColor("#d97706"));
        }
        m_table->setItem(i, 7, closItem);

        QString status;
        if (sc.closingQuantity == 0) {
            status = "⚠️ Empty";
        } else if (sc.closingQuantity < 10) {
            status = "⚡ Low";
        } else {
            status = "✓ OK";
        }

        auto* statusItem = new QTableWidgetItem(status);
        statusItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 8, statusItem);
    }
}

void StockCardTab::refresh() {}

// =================== ReportsWidget ===================

ReportsWidget::ReportsWidget(const User& user, QWidget* parent) : QWidget(parent), m_currentUser(user) { setupUi(); }

void ReportsWidget::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto* title = new QLabel("📊  Reports & Analytics");
    title->setObjectName("pageTitle");
    root->addWidget(title);

    m_tabs = new QTabWidget;

    m_dashTab = new DashboardTab;
    m_salesTab = new SalesReportTab;
    m_stockTab = new StockCardTab;

    m_tabs->addTab(m_dashTab, "📈  Dashboard");
    m_tabs->addTab(m_salesTab, "💰  Sales Report");
    m_tabs->addTab(m_stockTab, "📦  Stock Card");

    root->addWidget(m_tabs);
}

void ReportsWidget::refresh() {
    m_dashTab->refresh();
    m_salesTab->refresh();
    m_stockTab->refresh();
}
