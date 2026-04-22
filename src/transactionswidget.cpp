#include "transactionswidget.hpp"
#include <QDateTime>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>
#include "database.hpp"

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

    // Print functionality
    connect(printBtn, &QPushButton::clicked, this, [this] {
        // Simple print dialog using text
        QString receipt;
        receipt += "========================================\n";
        receipt += "           EPharmacy Receipt\n";
        receipt += "========================================\n";
        receipt += QString("Date: %1\n").arg(m_transaction.createdAt.toString("dd MMM yyyy  hh:mm:ss"));
        receipt += QString("Ref: #%1\n").arg(m_transaction.id);
        receipt += "----------------------------------------\n";
        for (const auto& item : m_transaction.items) {
            receipt += QString("%-20s x%2\n").arg(item.genericName).arg(item.quantity);
            receipt += QString("  @ %1  =  UGX %2\n").arg(item.sellingPrice, 0, 'f', 2).arg(item.subtotal(), 0, 'f', 2);
        }
        receipt += "========================================\n";
        receipt += QString("TOTAL: UGX %1\n").arg(m_transaction.total(), 0, 'f', 2);
        receipt += "========================================\n";
        receipt += "       Thank you for your purchase!\n";

        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Receipt Preview");
        msgBox.setText("Receipt preview (send to printer):");
        msgBox.setDetailedText(receipt);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    });
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
