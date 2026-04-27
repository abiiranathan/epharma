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
#include <QtCharts/QHorizontalBarSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPieSeries>
#include <QtCharts/QValueAxis>
#include "database.hpp"

static QString fmtCurrency(double v) { return QString("UGX %L1").arg(v, 0, 'f', 2); }

static QWidget* makeStatCard(const QString& title, QLabel*& valueOut, const QString& bg, const QString& fg) {
    auto* card = new QWidget;
    card->setObjectName("statsCard");
    card->setStyleSheet(QString("QWidget#statsCard { background-color: %1; border-radius: 10px; "
                                "border: 1px solid #e0e6ed; padding: 16px; }")
                            .arg(bg));
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(6);

    auto* lbl = new QLabel(title);
    lbl->setStyleSheet(
        "font-size:10px;font-weight:600;color:#6b7a99;"
        "text-transform:uppercase;letter-spacing:0.4px;");

    valueOut = new QLabel("—");
    valueOut->setStyleSheet(QString("font-size:18px;font-weight:700;color:%1;").arg(fg));

    layout->addWidget(lbl);
    layout->addWidget(valueOut);
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

    // ── Title ──────────────────────────────────────────────────────
    auto* title = new QLabel("Detailed Sales Report");
    title->setStyleSheet("font-size:15px;font-weight:700;color:#1e3a5f;");
    root->addWidget(title);

    // ── Controls ───────────────────────────────────────────────────
    auto* ctrlRow = new QHBoxLayout;
    ctrlRow->setSpacing(10);

    auto makeLabel = [](const QString& t) {
        auto* l = new QLabel(t);
        l->setStyleSheet("font-weight:600;color:#4a5568;");
        return l;
    };

    m_periodCombo = new QComboBox;
    m_periodCombo->addItems({"Daily", "Monthly", "Annual"});
    m_periodCombo->setFixedWidth(110);

    m_dateFrom = new QDateEdit(QDate::currentDate().addMonths(-1));
    m_dateFrom->setCalendarPopup(true);
    m_dateFrom->setDisplayFormat("yyyy-MM-dd");

    m_dateTo = new QDateEdit(QDate::currentDate());
    m_dateTo->setCalendarPopup(true);
    m_dateTo->setDisplayFormat("yyyy-MM-dd");

    auto* genBtn = new QPushButton("Generate Report");
    genBtn->setObjectName("successBtn");
    genBtn->setFixedHeight(34);

    ctrlRow->addWidget(makeLabel("Period:"));
    ctrlRow->addWidget(m_periodCombo);
    ctrlRow->addWidget(makeLabel("From:"));
    ctrlRow->addWidget(m_dateFrom);
    ctrlRow->addWidget(makeLabel("To:"));
    ctrlRow->addWidget(m_dateTo);
    ctrlRow->addStretch();
    ctrlRow->addWidget(genBtn);
    root->addLayout(ctrlRow);

    // ── Stat cards ─────────────────────────────────────────────────
    auto* statsRow = new QHBoxLayout;
    statsRow->setSpacing(8);
    statsRow->addWidget(makeStatCard("Total Income", m_statIncome, "#ebf8ff", "#2b6cb0"));
    statsRow->addWidget(makeStatCard("Total Profit", m_statProfit, "#f0fff4", "#276749"));
    statsRow->addWidget(makeStatCard("Profit Margin", m_statMargin, "#faf5ff", "#553c9a"));
    statsRow->addWidget(makeStatCard("Units Sold", m_statUnits, "#fffbeb", "#b7791f"));
    statsRow->addWidget(makeStatCard("Product Lines", m_statLines, "#fff5f5", "#c53030"));
    statsRow->addWidget(makeStatCard("Avg per Line", m_statAvgOrder, "#e6fffa", "#234e52"));
    root->addLayout(statsRow);

    // ── Charts row ─────────────────────────────────────────────────
    auto* chartsRow = new QHBoxLayout;
    chartsRow->setSpacing(10);

    auto makeChartView = [](QChartView*& cv) -> QChartView* {
        cv = new QChartView;
        cv->setRenderHint(QPainter::Antialiasing);
        cv->setMinimumHeight(220);
        cv->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        // placeholder empty chart
        auto* c = new QChart;
        c->setBackgroundBrush(Qt::white);
        c->setMargins(QMargins(8, 8, 8, 8));
        cv->setChart(c);
        return cv;
    };

    chartsRow->addWidget(makeChartView(m_incomeChartView), 2);
    chartsRow->addWidget(makeChartView(m_profitChartView), 2);
    chartsRow->addWidget(makeChartView(m_topProductsChartView), 2);
    root->addLayout(chartsRow);

    // ── Summary label ──────────────────────────────────────────────
    m_totalLabel = new QLabel("Generate a report to see results.");
    m_totalLabel->setStyleSheet(
        "font-size:13px;font-weight:600;color:#2f855a;"
        "background:#f0fff4;border-radius:6px;padding:7px 14px;");
    root->addWidget(m_totalLabel);

    // ── Table ──────────────────────────────────────────────────────
    m_table = new QTableWidget;
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({"Date", "Product", "Qty Sold", "Cost", "Selling Price", "Income", "Profit"});
    m_table->setMinimumHeight(250);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setColumnWidth(0, 120);
    m_table->setColumnWidth(2, 90);
    m_table->setColumnWidth(3, 120);
    m_table->setColumnWidth(4, 120);
    m_table->setColumnWidth(5, 130);
    m_table->setColumnWidth(6, 130);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    root->addWidget(m_table);

    connect(genBtn, &QPushButton::clicked, this, &SalesReportTab::onGenerate);
}

void SalesReportTab::updateCharts(const QList<ProductSale>& sales) {
    // ── Aggregate by date for income + profit trend ────────────────
    QMap<QDate, double> incomeByDate;
    QMap<QDate, double> profitByDate;
    QMap<QString, double> incomeByProduct;

    double totalIncome = 0, totalProfit = 0;
    int totalUnits = 0;

    for (const auto& s : sales) {
        incomeByDate[s.transactionDate] += s.income;
        profitByDate[s.transactionDate] += s.profit;
        incomeByProduct[s.productName] += s.income;
        totalIncome += s.income;
        totalProfit += s.profit;
        totalUnits += s.quantitySold;
    }

    // ── Stat cards ─────────────────────────────────────────────────
    double margin = totalIncome > 0 ? (totalProfit / totalIncome * 100.0) : 0.0;
    double avgLine = sales.isEmpty() ? 0.0 : totalIncome / static_cast<double>(sales.size());

    auto fmt = [](double v) { return QString("UGX %L1").arg(v, 0, 'f', 0); };

    m_statIncome->setText(fmt(totalIncome));
    m_statProfit->setText(fmt(totalProfit));
    m_statMargin->setText(QString("%1%").arg(margin, 0, 'f', 1));
    m_statUnits->setText(QString::number(totalUnits));
    m_statLines->setText(QString::number(sales.size()));
    m_statAvgOrder->setText(fmt(avgLine));

    // ── Chart 1: Income trend line ─────────────────────────────────
    {
        auto* incomeSeries = new QLineSeries;
        incomeSeries->setName("Income");
        incomeSeries->setPen(QPen(QColor("#3a7bd5"), 2.5));

        auto* profitSeries = new QLineSeries;
        profitSeries->setName("Profit");
        profitSeries->setPen(QPen(QColor("#2f855a"), 2.5));

        // Use sequential x-axis if many dates; datetime axis otherwise
        QList<QDate> dates = incomeByDate.keys();

        if (dates.size() <= 60) {
            // DateTime axis
            for (const QDate& d : dates) {
                qint64 ms = QDateTime(d, QTime(0, 0), QTimeZone::utc()).toMSecsSinceEpoch();
                incomeSeries->append((double)ms, incomeByDate[d]);
                profitSeries->append((double)ms, profitByDate.value(d, 0));
            }

            // Add an extra point at the end to make the last date's value visible on the right edge
            // Otherwise, if the last date is today and has sales, the line will end right at the edge and look cut off
            if (dates.size() == 1) {
                qint64 ms2 = QDateTime(dates[0].addDays(1), QTime(0, 0), QTimeZone::utc()).toMSecsSinceEpoch();
                incomeSeries->append((double)ms2, incomeByDate[dates[0]]);
                profitSeries->append((double)ms2, profitByDate.value(dates[0], 0));
            }

            auto* chart = new QChart;
            chart->setTitle("Income & Profit Trend");
            chart->setTitleFont(QFont("Segoe UI", 9, QFont::Bold));
            chart->setBackgroundBrush(Qt::white);
            chart->setAnimationOptions(QChart::SeriesAnimations);
            chart->setMargins(QMargins(8, 8, 8, 8));
            chart->addSeries(incomeSeries);
            chart->addSeries(profitSeries);

            auto* axisX = new QDateTimeAxis;
            axisX->setFormat(dates.size() > 30 ? "MMM yy" : "dd MMM");
            axisX->setLabelsFont(QFont("Segoe UI", 7));
            axisX->setTickCount(qMin(static_cast<int>(dates.size()), 8));
            chart->addAxis(axisX, Qt::AlignBottom);
            incomeSeries->attachAxis(axisX);
            profitSeries->attachAxis(axisX);

            auto* axisY = new QValueAxis;
            axisY->setLabelFormat("%.0f");
            axisY->setMin(0);  // prevent negative-only range on single point
            axisY->setLabelsFont(QFont("Segoe UI", 7));
            chart->addAxis(axisY, Qt::AlignLeft);
            incomeSeries->attachAxis(axisY);
            profitSeries->attachAxis(axisY);

            chart->legend()->setAlignment(Qt::AlignBottom);
            chart->legend()->setFont(QFont("Segoe UI", 8));
            m_incomeChartView->setChart(chart);

        } else {
            // Too many points — aggregate by month, use bar chart
            QMap<QString, double> byMonth;
            for (const QDate& d : dates) {
                QString key = d.toString("yyyy-MM");
                byMonth[key] += incomeByDate[d];
            }
            auto* set = new QBarSet("Income");
            set->setColor(QColor("#3a7bd5"));
            QStringList cats;
            for (auto it = byMonth.begin(); it != byMonth.end(); ++it) {
                cats << it.key();
                *set << it.value();
            }
            auto* series = new QBarSeries;
            series->append(set);

            auto* chart = new QChart;
            chart->setTitle("Income by Month");
            chart->setTitleFont(QFont("Segoe UI", 9, QFont::Bold));
            chart->setBackgroundBrush(Qt::white);
            chart->setMargins(QMargins(8, 8, 8, 8));
            chart->addSeries(series);

            auto* axisX = new QBarCategoryAxis;
            axisX->append(cats);
            axisX->setLabelsFont(QFont("Segoe UI", 7));
            chart->addAxis(axisX, Qt::AlignBottom);
            series->attachAxis(axisX);

            auto* axisY = new QValueAxis;
            axisY->setLabelFormat("%.0f");
            axisY->setLabelsFont(QFont("Segoe UI", 7));
            chart->addAxis(axisY, Qt::AlignLeft);
            series->attachAxis(axisY);

            chart->legend()->setVisible(false);
            m_incomeChartView->setChart(chart);
        }
    }

    // ── Chart 2: Daily profit bars ─────────────────────────────────
    {
        auto* chart = new QChart;
        chart->setTitle("Profit by Period");
        chart->setTitleFont(QFont("Segoe UI", 9, QFont::Bold));
        chart->setBackgroundBrush(Qt::white);
        chart->setAnimationOptions(QChart::SeriesAnimations);
        chart->setMargins(QMargins(8, 8, 8, 8));

        QList<QDate> dates = profitByDate.keys();
        bool useMonths = dates.size() > 31;

        if (useMonths) {
            QMap<QString, double> byMonth;
            for (const QDate& d : dates) {
                byMonth[d.toString("MMM yy")] += profitByDate[d];
            }
            auto* posSet = new QBarSet("Profit");
            posSet->setColor(QColor("#2f855a"));
            auto* negSet = new QBarSet("Loss");
            negSet->setColor(QColor("#e53e3e"));
            QStringList cats;
            for (auto it = byMonth.begin(); it != byMonth.end(); ++it) {
                cats << it.key();
                double v = it.value();
                *posSet << (v >= 0 ? v : 0);
                *negSet << (v < 0 ? -v : 0);
            }
            auto* series = new QBarSeries;
            series->append(posSet);
            if (negSet->at(0) > 0) series->append(negSet);
            chart->addSeries(series);

            auto* axisX = new QBarCategoryAxis;
            axisX->append(cats);
            axisX->setLabelsFont(QFont("Segoe UI", 7));
            chart->addAxis(axisX, Qt::AlignBottom);
            series->attachAxis(axisX);

            auto* axisY = new QValueAxis;
            axisY->setLabelFormat("%.0f");
            axisY->setLabelsFont(QFont("Segoe UI", 7));
            chart->addAxis(axisY, Qt::AlignLeft);
            series->attachAxis(axisY);
        } else {
            auto* profitSeries = new QLineSeries;
            profitSeries->setName("Profit");
            profitSeries->setPen(QPen(QColor("#2f855a"), 2.5));

            for (const QDate& d : dates) {
                qint64 ms = QDateTime(d, QTime(0, 0), QTimeZone::utc()).toMSecsSinceEpoch();
                profitSeries->append((double)ms, profitByDate[d]);
            }

            if (dates.size() == 1) {
                qint64 ms2 = QDateTime(dates[0].addDays(1), QTime(0, 0), QTimeZone::utc()).toMSecsSinceEpoch();
                profitSeries->append((double)ms2, profitByDate[dates[0]]);
            }

            chart->addSeries(profitSeries);

            auto* axisX = new QDateTimeAxis;
            axisX->setTickCount(qMax(2, qMin(static_cast<int>(dates.size()) + 1, 8)));
            axisX->setFormat("dd MMM");
            axisX->setLabelsFont(QFont("Segoe UI", 7));
            axisX->setTickCount(qMin(static_cast<int>(dates.size()), 8));
            chart->addAxis(axisX, Qt::AlignBottom);
            profitSeries->attachAxis(axisX);

            auto* axisY = new QValueAxis;
            axisY->setLabelFormat("%.0f");
            axisY->setLabelsFont(QFont("Segoe UI", 7));
            chart->addAxis(axisY, Qt::AlignLeft);
            profitSeries->attachAxis(axisY);
        }

        chart->legend()->setVisible(false);
        m_profitChartView->setChart(chart);
    }

    // ── Chart 3: Top 7 products by income (horizontal bar) ────────
    {
        // Sort by income descending, take top 7
        QList<QPair<QString, double>> sorted;
        for (auto it = incomeByProduct.begin(); it != incomeByProduct.end(); ++it) {
            sorted.append({it.key(), it.value()});
        }

        std::sort(sorted.begin(), sorted.end(),                                       // NOLINT
                  [](const auto& a, const auto& b) { return a.second > b.second; });  // NOLINT
        if (sorted.size() > 7) {
            sorted = sorted.mid(0, 7);
        }

        auto* set = new QBarSet("Income");
        set->setColor(QColor("#553c9a"));
        QStringList cats;

        // Reverse so highest is at top of horizontal chart
        for (int i = static_cast<int>(sorted.size() - 1); i >= 0; --i) {
            QString name = sorted[i].first;
            // shorten long names
            if (name.length() > 22) {
                name = name.left(19) + "…";
            }
            cats << name;
            *set << sorted[i].second;
        }

        auto* series = new QHorizontalBarSeries;
        series->append(set);

        auto* chart = new QChart;
        chart->setTitle("Top Products");
        chart->setTitleFont(QFont("Segoe UI", 9, QFont::Bold));
        chart->setBackgroundBrush(Qt::white);
        chart->setAnimationOptions(QChart::SeriesAnimations);
        chart->setMargins(QMargins(8, 8, 8, 8));
        chart->addSeries(series);

        auto* axisY = new QBarCategoryAxis;  // categories on Y for horizontal
        axisY->append(cats);
        axisY->setLabelsFont(QFont("Segoe UI", 7));
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);

        auto* axisX = new QValueAxis;
        axisX->setLabelFormat("%.0f");
        axisX->setLabelsFont(QFont("Segoe UI", 7));
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);

        chart->legend()->setVisible(false);
        m_topProductsChartView->setChart(chart);
    }
}

void SalesReportTab::onGenerate() {
    QList<ProductSale> sales;
    QString period = m_periodCombo->currentText();
    QDate from = m_dateFrom->date();
    QDate to = m_dateTo->date();

    if (period == "Daily") {
        for (QDate d = from; d <= to; d = d.addDays(1)) {
            sales += Database::instance().getDailyProductSales(d);
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

    // ── Update charts & stat cards ─────────────────────────────────
    updateCharts(sales);

    // ── Populate table ─────────────────────────────────────────────
    m_table->setRowCount(0);
    m_table->setRowCount(static_cast<int>(sales.size()));
    double totalIncome = 0, totalProfit = 0;

    for (int i = 0; i < sales.size(); ++i) {
        const auto& s = sales[i];
        m_table->setItem(i, 0, new QTableWidgetItem(s.transactionDate.toString("dd MMM yyyy")));
        m_table->setItem(i, 1, new QTableWidgetItem(s.productName));

        auto* qtyItem = new QTableWidgetItem(QString::number(s.quantitySold));
        qtyItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 2, qtyItem);

        auto makeRight = [](double v, const QString& color = "") -> QTableWidgetItem* {
            auto* item = new QTableWidgetItem(QString("UGX %1").arg(v, 0, 'f', 2));
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            if (!color.isEmpty()) item->setForeground(QColor(color));
            return item;
        };

        m_table->setItem(i, 3, makeRight(s.costPrice));
        m_table->setItem(i, 4, makeRight(s.sellingPrice));

        auto* incItem = makeRight(s.income, "#2b6cb0");
        m_table->setItem(i, 5, incItem);

        auto* profItem = makeRight(s.profit, s.profit >= 0 ? "#2f855a" : "#e53e3e");
        profItem->setFont(QFont("", -1, QFont::Bold));
        m_table->setItem(i, 6, profItem);

        totalIncome += s.income;
        totalProfit += s.profit;
    }

    m_totalLabel->setText(QString("Income: UGX %L1   |   Profit: UGX %L2   |   Items: %3")
                              .arg(totalIncome, 0, 'f', 2)
                              .arg(totalProfit, 0, 'f', 2)
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
