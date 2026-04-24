#include "transactionswidget.hpp"
#include <QDateTime>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPainter>
#include <QPrintDialog>
#include <QPrinter>
#include <QVBoxLayout>
#include "database.hpp"

// =================== Receipt Printer ===================

static void printReceipt(const Transaction& t, QWidget* parent) {
    QPrinter printer(QPrinter::HighResolution);

    // Default to receipt paper: 80mm wide (most common thermal receipt printer)
    // 80mm = ~226 points (1 pt = 1/72 inch, 80mm / 25.4 * 72 ≈ 227)
    // Height is set long enough; printer will cut at end of content.
    printer.setPageSize(QPageSize(QSizeF(80, 297), QPageSize::Millimeter));
    printer.setPageMargins(QMarginsF(4, 4, 4, 4), QPageLayout::Millimeter);
    printer.setFullPage(false);
    printer.setColorMode(QPrinter::GrayScale);

    QPrintDialog dlg(&printer, parent);
    dlg.setWindowTitle("Print Receipt — EPharmacy");
    // Allow printer selection and page setup
    dlg.setOptions(QAbstractPrintDialog::PrintToFile | QAbstractPrintDialog::PrintShowPageSize);

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    QPainter p;
    if (!p.begin(&printer)) {
        QMessageBox::critical(parent, "Print Error", "Failed to open printer.");
        return;
    }

    // ---- Coordinate system ----
    // Work in printer points. Get the usable rect.
    const QRectF pageRect = printer.pageLayout().paintRectPixels(printer.resolution());
    const qreal W = pageRect.width();
    const qreal dpi = printer.resolution();  // dots per inch
    const qreal mm = dpi / 25.4;             // 1mm in dots

    // Fonts scaled for receipt printer
    QFont fTitle("Courier New", -1, QFont::Bold);
    fTitle.setPixelSize(qRound(4.0 * mm));  // ~4mm tall

    QFont fHeader("Courier New", -1, QFont::Bold);
    fHeader.setPixelSize(qRound(3.2 * mm));

    QFont fNormal("Courier New");
    fNormal.setPixelSize(qRound(3.0 * mm));

    QFont fSmall("Courier New");
    fSmall.setPixelSize(qRound(2.5 * mm));

    QFont fTotal("Courier New", -1, QFont::Bold);
    fTotal.setPixelSize(qRound(4.5 * mm));

    const qreal lineH = 3.6 * mm;    // normal line height
    const qreal lineHS = 2.9 * mm;   // small line height
    const qreal dotLine = 0.3 * mm;  // divider thickness

    qreal y = 0;

    auto drawHRule = [&](bool bold = false) {
        p.setPen(QPen(Qt::black, bold ? dotLine * 2 : dotLine));
        p.drawLine(QPointF(0, y), QPointF(W, y));
        y += 1.5 * mm;
    };

    auto drawCentered = [&](const QString& text, const QFont& font, qreal h) {
        p.setFont(font);
        p.setPen(Qt::black);
        p.drawText(QRectF(0, y, W, h), Qt::AlignHCenter | Qt::AlignVCenter, text);
        y += h;
    };

    auto drawRow = [&](const QString& left, const QString& right, const QFont& font, qreal h) {
        p.setFont(font);
        p.setPen(Qt::black);
        p.drawText(QRectF(0, y, W * 0.65, h), Qt::AlignLeft | Qt::AlignVCenter, left);
        p.drawText(QRectF(0, y, W, h), Qt::AlignRight | Qt::AlignVCenter, right);
        y += h;
    };

    // ---- Header ----
    y += 2 * mm;
    drawCentered("EPharmacy", fTitle, 5 * mm);
    drawCentered("Sales Receipt", fNormal, lineH);
    y += 1.5 * mm;
    drawHRule(true);

    // ---- Meta ----
    drawRow("Date:", t.createdAt.toString("dd/MM/yyyy hh:mm"), fSmall, lineHS);
    drawRow("Ref #:", QString::number(t.id), fSmall, lineHS);
    y += 1 * mm;
    drawHRule();

    // ---- Column headers ----
    p.setFont(fHeader);
    p.setPen(Qt::black);
    p.drawText(QRectF(0, y, W * 0.55, lineH), Qt::AlignLeft | Qt::AlignVCenter, "Item");
    p.drawText(QRectF(W * 0.55, y, W * 0.13, lineH), Qt::AlignRight | Qt::AlignVCenter, "Qty");
    p.drawText(QRectF(W * 0.68, y, W * 0.16, lineH), Qt::AlignRight | Qt::AlignVCenter, "Price");
    p.drawText(QRectF(W * 0.84, y, W * 0.16, lineH), Qt::AlignRight | Qt::AlignVCenter, "Sub");
    y += lineH;
    drawHRule();

    // ---- Items ----
    double grandTotal = 0.0;
    for (const auto& item : t.items) {
        const double sub = item.subtotal();
        grandTotal += sub;

        // Product name — may need to wrap on narrow paper
        QString name = item.genericName;
        if (!item.brandName.isEmpty()) {
            name += " (" + item.brandName + ")";
        }

        // Measure name width; wrap if too wide
        QFontMetricsF fm(fNormal);
        const qreal maxNameW = W * 0.54;
        if (fm.horizontalAdvance(name) > maxNameW) {
            // Try generic name alone first
            if (fm.horizontalAdvance(item.genericName) <= maxNameW) {
                name = item.genericName;
            } else {
                // Elide
                name = fm.elidedText(item.genericName, Qt::ElideRight, static_cast<int>(maxNameW));
            }
        }

        p.setFont(fNormal);
        p.setPen(Qt::black);
        p.drawText(QRectF(0, y, W * 0.55, lineH), Qt::AlignLeft | Qt::AlignVCenter, name);
        p.drawText(QRectF(W * 0.55, y, W * 0.13, lineH), Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(item.quantity));
        p.drawText(QRectF(W * 0.68, y, W * 0.16, lineH), Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(item.sellingPrice, 'f', 0));
        p.drawText(QRectF(W * 0.84, y, W * 0.16, lineH), Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(sub, 'f', 0));
        y += lineH;
    }

    // ---- Total ----
    y += 1 * mm;
    drawHRule(true);
    drawRow("TOTAL (UGX)", QString::number(grandTotal, 'f', 2), fTotal, 5.5 * mm);
    drawHRule(true);

    // ---- Footer ----
    y += 2 * mm;
    drawCentered("Thank you for your purchase!", fSmall, lineHS);
    drawCentered("EPharmacy — Serving you better", fSmall, lineHS);
    y += 3 * mm;

    p.end();
}

// =================== TransactionDetailWidget ===================

TransactionDetailWidget::TransactionDetailWidget(const Transaction& t, const User& user, QWidget* parent)
    : QWidget(parent), m_transaction(t), m_user(user) {
    setupUi();
}

void TransactionDetailWidget::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto* headerRow = new QHBoxLayout;
    auto* backBtn = new QPushButton("← Back to Transactions");
    backBtn->setObjectName("secondaryBtn");
    backBtn->setFixedHeight(32);
    connect(backBtn, &QPushButton::clicked, this, &TransactionDetailWidget::backRequested);

    auto* title = new QLabel(QString("Transaction #%1").arg(m_transaction.id));
    title->setObjectName("pageTitle");
    headerRow->addWidget(backBtn);
    headerRow->addSpacing(12);
    headerRow->addWidget(title);
    headerRow->addStretch();

    // Print button
    auto* printBtn = new QPushButton("🖨  Print Receipt");
    printBtn->setObjectName("successBtn");
    printBtn->setFixedHeight(32);
    headerRow->addWidget(printBtn);
    root->addLayout(headerRow);

    // Info row
    auto* infoGroup = new QGroupBox("Transaction Info");
    auto* infoLayout = new QHBoxLayout(infoGroup);

    auto addField = [&](const QString& label, const QString& val, const QString& color = "#1e3a5f") {
        auto* col = new QVBoxLayout;
        auto* lbl = new QLabel(label);
        lbl->setStyleSheet("color: #718096; font-size: 11px; font-weight: 600;");
        auto* v = new QLabel(val);
        v->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: 600;").arg(color));
        col->addWidget(lbl);
        col->addWidget(v);
        infoLayout->addLayout(col);
        infoLayout->addSpacing(24);
    };

    addField("TRANSACTION ID", QString("#%1").arg(m_transaction.id));
    addField("DATE & TIME", m_transaction.createdAt.toString("dd MMM yyyy  hh:mm:ss"));
    addField("ITEMS", QString::number(m_transaction.items.size()));
    addField("TOTAL", QString("UGX %L1").arg(m_transaction.total(), 0, 'f', 2), "#2f855a");
    infoLayout->addStretch();
    root->addWidget(infoGroup);

    // Items table
    auto* tableLabel = new QLabel("Items");
    tableLabel->setStyleSheet("font-size: 14px; font-weight: 600; color: #2d3748;");
    root->addWidget(tableLabel);

    auto* tbl = new QTableWidget;
    tbl->setColumnCount(6);
    tbl->setHorizontalHeaderLabels({"Product", "Brand", "Barcode", "Price", "Qty", "Subtotal"});
    tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tbl->setAlternatingRowColors(true);
    tbl->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    tbl->setColumnWidth(1, 120);
    tbl->setColumnWidth(2, 120);
    tbl->setColumnWidth(3, 90);
    tbl->setColumnWidth(4, 55);
    tbl->setColumnWidth(5, 100);
    tbl->verticalHeader()->setVisible(false);
    tbl->setShowGrid(false);
    tbl->setRowCount(static_cast<int>(m_transaction.items.size()));

    double total = 0;
    for (int i = 0; i < m_transaction.items.size(); ++i) {
        const auto& item = m_transaction.items[i];
        tbl->setItem(i, 0, new QTableWidgetItem(item.genericName));
        tbl->setItem(i, 1, new QTableWidgetItem(item.brandName));
        tbl->setItem(i, 2, new QTableWidgetItem(item.barcode));

        auto* priceItem = new QTableWidgetItem(QString::number(item.sellingPrice, 'f', 2));
        priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        tbl->setItem(i, 3, priceItem);

        auto* qtyItem = new QTableWidgetItem(QString::number(item.quantity));
        qtyItem->setTextAlignment(Qt::AlignCenter);
        tbl->setItem(i, 4, qtyItem);

        double sub = item.subtotal();
        total += sub;
        auto* subItem = new QTableWidgetItem(QString("UGX %1").arg(sub, 0, 'f', 2));
        subItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        subItem->setFont(QFont("", -1, QFont::Medium));
        tbl->setItem(i, 5, subItem);
    }
    root->addWidget(tbl);

    // Total row
    auto* totalRow = new QHBoxLayout;
    totalRow->addStretch();
    auto* totalLabel = new QLabel(QString("GRAND TOTAL:  UGX %L1").arg(total, 0, 'f', 2));
    totalLabel->setStyleSheet(
        "color: white; background-color: #1e3a5f; font-size: 18px; font-weight: 700;"
        "border-radius: 8px; padding: 10px 20px;");
    totalRow->addWidget(totalLabel);
    root->addLayout(totalRow);

    // Cancel button (only within 1 hour)
    qint64 elapsed = m_transaction.createdAt.secsTo(QDateTime::currentDateTime());
    if (elapsed <= 3600) {
        auto* cancelBtn = new QPushButton("🚫  Cancel Transaction");
        cancelBtn->setObjectName("dangerBtn");
        cancelBtn->setFixedHeight(36);
        cancelBtn->setFixedWidth(200);
        root->addWidget(cancelBtn);

        int tid = m_transaction.id;
        connect(cancelBtn, &QPushButton::clicked, this, [this, tid] {
            auto ret =
                QMessageBox::question(this, "Cancel Transaction", "Cancel this transaction? Stock will be restored.",
                                      QMessageBox::Yes | QMessageBox::No);
            if (ret == QMessageBox::Yes) {
                if (!Database::instance().deleteTransaction(tid)) {
                    QMessageBox::critical(this, "Error", Database::instance().lastError());
                } else {
                    QMessageBox::information(this, "Cancelled", "Transaction cancelled and stock restored.");
                    emit transactionDeleted();
                    emit backRequested();
                }
            }
        });
    } else {
        auto* note = new QLabel("⚠️  Transactions can only be cancelled within 1 hour.");
        note->setStyleSheet("color: #a0aec0; font-size: 12px;");
        root->addWidget(note);
    }

    // ---- Print receipt ----
    connect(printBtn, &QPushButton::clicked, this, [this] { printReceipt(m_transaction, this); });
}

// =================== TransactionsWidget ===================

TransactionsWidget::TransactionsWidget(const User& user, QWidget* parent) : QWidget(parent), m_currentUser(user) {
    setupUi();
}

void TransactionsWidget::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    m_stack = new QStackedWidget;

    // List page
    auto* listPage = new QWidget;
    auto* listLayout = new QVBoxLayout(listPage);
    listLayout->setContentsMargins(16, 16, 16, 16);
    listLayout->setSpacing(12);

    auto* title = new QLabel("🧾  Transactions");
    title->setObjectName("pageTitle");
    listLayout->addWidget(title);

    auto* hint = new QLabel("Double-click a row to view full details. Transactions within 1 hour can be cancelled.");
    hint->setStyleSheet("color: #718096; font-size: 12px;");
    listLayout->addWidget(hint);

    m_table = new QTableWidget;
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"#", "Date & Time", "Items", "Total", "Action"});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setColumnWidth(0, 60);
    m_table->setColumnWidth(2, 80);
    m_table->setColumnWidth(3, 140);
    m_table->setColumnWidth(4, 90);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    listLayout->addWidget(m_table);

    m_stack->addWidget(listPage);
    root->addWidget(m_stack);

    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        auto* item = m_table->item(row, 0);
        if (item) {
            onViewDetails(item->data(Qt::UserRole).toInt());
        }
    });
}

void TransactionsWidget::refresh() {
    m_stack->setCurrentIndex(0);
    loadData();
}

void TransactionsWidget::loadData() {
    QList<Transaction> transactions = Database::instance().listTransactions(200, 0);
    m_table->setRowCount(0);
    m_table->setRowCount(static_cast<int>(transactions.size()));
    for (int i = 0; i < transactions.size(); ++i) {
        setRow(i, transactions[i]);
    }
}

void TransactionsWidget::setRow(int row, const Transaction& t) {
    auto* idItem = new QTableWidgetItem(QString("#%1").arg(t.id));
    idItem->setData(Qt::UserRole, t.id);
    idItem->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(row, 0, idItem);

    m_table->setItem(row, 1, new QTableWidgetItem(t.createdAt.toString("dd MMM yyyy  hh:mm")));

    auto* itemsItem = new QTableWidgetItem(QString::number(t.items.size()));
    itemsItem->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(row, 2, itemsItem);

    auto* totalItem = new QTableWidgetItem(QString("UGX %L1").arg(t.total(), 0, 'f', 2));
    totalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    totalItem->setForeground(QColor("#2f855a"));
    totalItem->setFont(QFont("", -1, QFont::Bold));
    m_table->setItem(row, 3, totalItem);

    auto* viewBtn = new QPushButton("Details");
    viewBtn->setFixedHeight(26);
    viewBtn->setStyleSheet(
        "QPushButton { background-color: #3a7bd5; color: white; border: none; "
        "border-radius: 3px; font-size: 12px; padding: 0 8px; }"
        "QPushButton:hover { background-color: #2d6bc4; }");
    int tid = t.id;
    connect(viewBtn, &QPushButton::clicked, this, [this, tid] { onViewDetails(tid); });
    m_table->setCellWidget(row, 4, viewBtn);
}

void TransactionsWidget::onViewDetails(int id) {
    Transaction t = Database::instance().getTransactionById(id);
    if (t.id == 0) {
        return;
    }

    if (m_detailWidget) {
        m_stack->removeWidget(m_detailWidget);
        delete m_detailWidget;
        m_detailWidget = nullptr;
    }

    m_detailWidget = new TransactionDetailWidget(t, m_currentUser);
    connect(m_detailWidget, &TransactionDetailWidget::backRequested, this, [this] {
        m_stack->setCurrentIndex(0);
        loadData();
    });
    connect(m_detailWidget, &TransactionDetailWidget::transactionDeleted, this, [this] { loadData(); });
    m_stack->addWidget(m_detailWidget);
    m_stack->setCurrentWidget(m_detailWidget);
}
