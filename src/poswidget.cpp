#include "poswidget.hpp"
#include <QComboBox>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLocale>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QShortcut>
#include <QSpinBox>
#include <QSplitter>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include "database.hpp"

POSWidget::POSWidget(User user, QWidget* parent) : QWidget(parent), m_currentUser(std::move(user)) {
    setupUi();
    loadProducts();
}

void POSWidget::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    // Title
    auto* titleRow = new QHBoxLayout;
    auto* title = new QLabel("🛒  Point of Sale");
    title->setObjectName("pageTitle");
    titleRow->addWidget(title);
    titleRow->addStretch();

    // Barcode input in title row
    auto* barcodeLabel = new QLabel("Scan Barcode:");
    barcodeLabel->setStyleSheet("font-weight: 600; color: #4a5568;");
    m_barcodeEdit = new QLineEdit;
    m_barcodeEdit->setPlaceholderText("Scan or type barcode...");
    m_barcodeEdit->setObjectName("searchBar");
    m_barcodeEdit->setFixedWidth(220);
    m_barcodeEdit->setFixedHeight(34);
    titleRow->addWidget(barcodeLabel);
    titleRow->addWidget(m_barcodeEdit);
    root->addLayout(titleRow);

    // Splitter
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setHandleWidth(6);

    // ---- LEFT: Product browser ----
    auto* leftWidget = new QWidget;
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    auto* searchRow = new QHBoxLayout;
    auto* searchLabel = new QLabel("Search Product:");
    searchLabel->setStyleSheet("font-weight: 600; color: #4a5568;");
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("Type to search by name...");
    m_searchEdit->setObjectName("searchBar");
    m_searchEdit->setFixedHeight(34);
    searchRow->addWidget(searchLabel);
    searchRow->addWidget(m_searchEdit);
    leftLayout->addLayout(searchRow);

    m_productsTable = new QTableWidget;
    m_productsTable->setColumnCount(6);
    m_productsTable->setHorizontalHeaderLabels({"ID", "Generic Name", "Brand", "Price", "Stock", "Expiry"});
    m_productsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_productsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_productsTable->setAlternatingRowColors(true);
    m_productsTable->horizontalHeader()->setStretchLastSection(false);
    m_productsTable->verticalHeader()->setVisible(false);
    m_productsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);       // lock all first
    m_productsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);  // only Generic Name stretches

    m_productsTable->setColumnWidth(0, 40);  // ID
    // column 1 stretches — no setColumnWidth needed
    m_productsTable->setColumnWidth(2, 180);  // Brand
    m_productsTable->setColumnWidth(3, 120);  // Price
    m_productsTable->setColumnWidth(4, 80);   // Stock
    m_productsTable->setColumnWidth(5, 100);  // Expiry

    m_productsTable->setSelectionMode(QAbstractItemView::SingleSelection);

    auto* addHint = new QLabel("Double-click or press Enter to add to bill");
    addHint->setStyleSheet("color: #a0aec0; font-size: 11px;");

    leftLayout->addWidget(m_productsTable);
    leftLayout->addWidget(addHint);

    // ---- RIGHT: Sales Queue ----
    auto* rightWidget = new QWidget;
    rightWidget->setObjectName("receiptWidget");
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(12, 12, 12, 12);
    rightLayout->setSpacing(8);

    auto* receiptTitle = new QLabel("🧾  Sales Receipt");
    receiptTitle->setStyleSheet("font-size: 16px; font-weight: 700; color: #1e3a5f;");
    rightLayout->addWidget(receiptTitle);

    m_queueTable = new QTableWidget;
    m_queueTable->setColumnCount(6);
    m_queueTable->setHorizontalHeaderLabels({"Product", "Brand", "Price", "Qty", "Subtotal", "✕"});
    m_queueTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_queueTable->setAlternatingRowColors(true);
    m_queueTable->horizontalHeader()->setStretchLastSection(false);
    m_queueTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_queueTable->setColumnWidth(1, 100);
    m_queueTable->setColumnWidth(2, 80);
    m_queueTable->setColumnWidth(3, 55);
    m_queueTable->setColumnWidth(4, 90);
    m_queueTable->setColumnWidth(5, 36);
    m_queueTable->verticalHeader()->setVisible(false);
    m_queueTable->setShowGrid(false);
    rightLayout->addWidget(m_queueTable);

    // Total
    auto* totalWidget = new QWidget;
    totalWidget->setObjectName("receiptTotal");
    auto* totalLayout = new QHBoxLayout(totalWidget);
    totalLayout->setContentsMargins(16, 10, 16, 10);
    auto* totalTextLabel = new QLabel("TOTAL:");
    m_totalLabel = new QLabel("UGX 0.00");
    m_totalLabel->setStyleSheet("font-size: 18px; font-weight: 700;");
    totalLayout->addWidget(totalTextLabel);
    totalLayout->addStretch();
    totalLayout->addWidget(m_totalLabel);
    rightLayout->addWidget(totalWidget);

    // Buttons
    auto* btnRow = new QHBoxLayout;
    m_clearBtn = new QPushButton("🗑  Clear");
    m_clearBtn->setObjectName("secondaryBtn");
    m_clearBtn->setFixedHeight(38);
    m_saveBtn = new QPushButton("💾  Save Transaction");
    m_saveBtn->setObjectName("successBtn");
    m_saveBtn->setFixedHeight(38);
    btnRow->addWidget(m_clearBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_saveBtn);
    rightLayout->addLayout(btnRow);

    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    root->addWidget(splitter);

    // Connect signals
    connect(m_searchEdit, &QLineEdit::textChanged, this, &POSWidget::onSearchChanged);
    connect(m_productsTable, &QTableWidget::cellDoubleClicked, this, &POSWidget::onProductDoubleClicked);
    connect(m_saveBtn, &QPushButton::clicked, this, &POSWidget::saveTransaction);
    connect(m_clearBtn, &QPushButton::clicked, this, &POSWidget::clearQueue);
    connect(m_queueTable, &QTableWidget::cellChanged, this, &POSWidget::updateQueueSubtotal);

    // Enter key on product table
    auto* enterShortcut = new QShortcut(QKeySequence(Qt::Key_Return), m_productsTable);
    connect(enterShortcut, &QShortcut::activated, this, [this] {
        int row = m_productsTable->currentRow();
        if (row >= 0 && row < m_currentProducts.size()) {
            addProductToQueue(m_currentProducts[row]);
        }
    });

    m_barcodeTimer = new QTimer(this);
    m_barcodeTimer->setSingleShot(true);
    m_barcodeTimer->setInterval(100);  // 100ms gap = end of scan burst
    connect(m_barcodeTimer, &QTimer::timeout, this, &POSWidget::onBarcodeEntered);
    connect(m_barcodeEdit, &QLineEdit::returnPressed, this, &POSWidget::onBarcodeEntered);

    // Install event filter to capture barcode scanner input globally
    m_productsTable->installEventFilter(this);
}

void POSWidget::loadProducts(const QString& filter) {
    m_currentProducts = Database::instance().searchProducts(filter, 100);
    m_productsTable->setRowCount(0);
    m_productsTable->setRowCount(static_cast<int>(m_currentProducts.size()));

    for (int i = 0; i < m_currentProducts.size(); ++i) {
        const Product& p = m_currentProducts[i];

        auto* idItem = new QTableWidgetItem(QString::number(p.id));
        idItem->setTextAlignment(Qt::AlignCenter);
        m_productsTable->setItem(i, 0, idItem);
        m_productsTable->setItem(i, 1, new QTableWidgetItem(p.genericName));
        m_productsTable->setItem(i, 2, new QTableWidgetItem(p.brandName));

        auto* priceItem = new QTableWidgetItem(QString::number(p.sellingPrice, 'f', 2));
        priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_productsTable->setItem(i, 3, priceItem);

        auto* qtyItem = new QTableWidgetItem(QString::number(p.quantity));
        qtyItem->setTextAlignment(Qt::AlignCenter);
        if (p.quantity == 0) {
            qtyItem->setForeground(Qt::red);
        } else if (p.quantity < 10) {
            qtyItem->setForeground(QColor("#d97706"));
        }
        m_productsTable->setItem(i, 4, qtyItem);

        // Expiry dates
        QStringList dates;
        for (const auto& d : p.expiryDates) {
            dates << d.toString("MMM yyyy");
        }
        m_productsTable->setItem(i, 5, new QTableWidgetItem(dates.join(", ")));

        if (p.quantity == 0) {
            for (int c = 0; c < 6; ++c) {
                if (m_productsTable->item(i, c)) {
                    m_productsTable->item(i, c)->setBackground(QColor("#fff5f5"));
                }
            }
        }
    }
}

void POSWidget::onSearchChanged(const QString& text) { loadProducts(text); }

bool POSWidget::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(event);

        // Let ALL text-input widgets handle their own keys
        if (qobject_cast<QLineEdit*>(obj) || qobject_cast<QTextEdit*>(obj) || qobject_cast<QPlainTextEdit*>(obj) ||
            qobject_cast<QSpinBox*>(obj) || qobject_cast<QDoubleSpinBox*>(obj) || qobject_cast<QComboBox*>(obj) ||
            qobject_cast<QDateEdit*>(obj)) {
            return QWidget::eventFilter(obj, event);
        }

        QString ch = ke->text();

        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            if (!m_barcodeBuffer.isEmpty()) {
                m_barcodeEdit->setText(m_barcodeBuffer);
                m_barcodeBuffer.clear();
                m_barcodeTimer->stop();
                onBarcodeEntered();
                return true;
            }
            return false;
        }

        if (!ch.isEmpty() && ch[0].isPrint()) {
            m_barcodeBuffer += ch;
            m_barcodeTimer->start();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void POSWidget::onBarcodeEntered() {
    QString barcode = m_barcodeEdit->text().trimmed();
    if (barcode.isEmpty()) {
        return;
    }

    m_barcodeBuffer.clear();  // safety clear
    Product p = Database::instance().getProductByBarcode(barcode);
    m_barcodeEdit->clear();

    if (p.id == 0) {
        QMessageBox::warning(this, "Not Found", "No product found with barcode: " + barcode);
        return;
    }
    if (p.quantity == 0) {
        QMessageBox::warning(this, "Out of Stock", QString("'%1' is out of stock!").arg(p.genericName));
        return;
    }
    addProductToQueue(p);
}

void POSWidget::onProductDoubleClicked(int row, int /*unused*/) {
    if (row < 0 || row >= m_currentProducts.size()) {
        return;
    }
    const Product& p = m_currentProducts[row];
    if (p.quantity == 0) {
        QMessageBox::warning(this, "Out of Stock", QString("'%1' is out of stock!").arg(p.genericName));
        return;
    }
    addProductToQueue(p);
}

void POSWidget::addProductToQueue(const Product& product) {
    // Check if already in queue
    for (int i = 0; i < m_queueTable->rowCount(); ++i) {
        QTableWidgetItem* idItem = m_queueTable->item(i, 0);
        if (!idItem) {
            continue;
        }
        if (idItem->data(Qt::UserRole).toInt() == product.id) {
            // Increment quantity
            auto* qtyItem = m_queueTable->item(i, 3);
            if (!qtyItem) {
                return;
            }
            int currentQty = qtyItem->text().toInt();
            if (currentQty >= product.quantity) {
                QMessageBox::warning(this, "Insufficient Stock",
                                     QString("Maximum available quantity is %1").arg(product.quantity));
                return;
            }
            qtyItem->setText(QString::number(currentQty + 1));
            return;
        }
    }

    // Add new row
    int row = m_queueTable->rowCount();
    m_queueTable->insertRow(row);

    auto* nameItem = new QTableWidgetItem(product.genericName);
    nameItem->setData(Qt::UserRole, product.id);
    nameItem->setFlags(nameItem->flags() & ~static_cast<Qt::ItemFlags>(Qt::ItemIsEditable));
    m_queueTable->setItem(row, 0, nameItem);

    auto* brandItem = new QTableWidgetItem(product.brandName);
    brandItem->setFlags(brandItem->flags() & ~static_cast<Qt::ItemFlags>(Qt::ItemIsEditable));
    m_queueTable->setItem(row, 1, brandItem);

    auto* priceItem = new QTableWidgetItem(QString::number(product.sellingPrice, 'f', 2));
    priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    priceItem->setData(Qt::UserRole + 1, product.sellingPrice);
    priceItem->setFlags(priceItem->flags() & ~static_cast<Qt::ItemFlags>(Qt::ItemIsEditable));
    m_queueTable->setItem(row, 2, priceItem);

    auto* qtyItem = new QTableWidgetItem("1");
    qtyItem->setTextAlignment(Qt::AlignCenter);
    qtyItem->setData(Qt::UserRole + 2, product.quantity);  // store max qty
    m_queueTable->setItem(row, 3, qtyItem);

    auto* subtotalItem = new QTableWidgetItem(QString::number(product.sellingPrice, 'f', 2));
    subtotalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    subtotalItem->setFlags(subtotalItem->flags() & ~static_cast<Qt::ItemFlags>(Qt::ItemIsEditable));
    m_queueTable->setItem(row, 4, subtotalItem);

    // Remove button
    auto* removeBtn = new QPushButton("✕");
    removeBtn->setObjectName("dangerBtn");
    removeBtn->setFixedSize(28, 28);
    removeBtn->setStyleSheet(
        "QPushButton { background-color: #fc8181; color: white; border: none; "
        "border-radius: 3px; font-size: 12px; font-weight: bold; padding: 0; }"
        "QPushButton:hover { background-color: #e53e3e; }");
    int r = row;
    connect(removeBtn, &QPushButton::clicked, this, [this, r] { removeFromQueue(r); });
    m_queueTable->setCellWidget(row, 5, removeBtn);

    updateTotal();
}

void POSWidget::removeFromQueue(int row) {
    if (row < 0 || row >= m_queueTable->rowCount()) {
        return;
    }
    m_queueTable->removeRow(row);
    // Reconnect remove buttons with updated row indices
    for (int i = 0; i < m_queueTable->rowCount(); ++i) {
        auto* btn = qobject_cast<QPushButton*>(m_queueTable->cellWidget(i, 5));
        if (btn) {
            btn->disconnect();
            int r = i;
            connect(btn, &QPushButton::clicked, this, [this, r] { removeFromQueue(r); });
        }
    }
    updateTotal();
}

void POSWidget::updateQueueSubtotal(int row, int col) {
    if (col != 3) {
        return;  // Only quantity column
    }
    auto* qtyItem = m_queueTable->item(row, 3);
    auto* priceItem = m_queueTable->item(row, 2);
    auto* subtotalItem = m_queueTable->item(row, 4);
    if (!qtyItem || !priceItem || !subtotalItem) {
        return;
    }

    int qty = qtyItem->text().toInt();
    int maxQty = qtyItem->data(Qt::UserRole + 2).toInt();
    qty = std::max(qty, 1);

    if (qty > maxQty) {
        qty = maxQty;
        QMessageBox::warning(this, "Insufficient Stock", QString("Maximum available quantity is %1").arg(maxQty));
    }

    // Block signals to avoid recursion
    m_queueTable->blockSignals(true);
    qtyItem->setText(QString::number(qty));
    double price = priceItem->data(Qt::UserRole + 1).toDouble();
    subtotalItem->setText(QString::number(price * qty, 'f', 2));
    m_queueTable->blockSignals(false);

    updateTotal();
}

double POSWidget::computeTotal() const {
    double total = 0.0;
    for (int i = 0; i < m_queueTable->rowCount(); ++i) {
        auto* sub = m_queueTable->item(i, 4);
        if (sub) {
            total += sub->text().toDouble();
        }
    }
    return total;
}

void POSWidget::updateTotal() { m_totalLabel->setText(formatCurrency(computeTotal())); }

void POSWidget::clearQueue() {
    if (m_queueTable->rowCount() == 0) {
        return;
    }
    auto ret = QMessageBox::question(this, "Clear Receipt", "Clear all items from the receipt?",
                                     QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        m_queueTable->setRowCount(0);
        updateTotal();
    }
}

void POSWidget::saveTransaction() {
    if (m_queueTable->rowCount() == 0) {
        QMessageBox::warning(this, "Empty Receipt", "No items in the sales receipt.");
        return;
    }

    Transaction t;
    t.userId = m_currentUser.id;
    t.createdAt = QDateTime::currentDateTime();

    for (int i = 0; i < m_queueTable->rowCount(); ++i) {
        auto* nameItem = m_queueTable->item(i, 0);
        auto* brandItem = m_queueTable->item(i, 1);
        auto* priceItem = m_queueTable->item(i, 2);
        auto* qtyItem = m_queueTable->item(i, 3);
        if (!nameItem || !qtyItem || !priceItem) {
            continue;
        }

        int qty = qtyItem->text().toInt();
        if (qty <= 0) {
            continue;
        }

        TransactionItem item;
        item.productId = nameItem->data(Qt::UserRole).toInt();
        item.genericName = nameItem->text();
        item.brandName = brandItem ? brandItem->text() : QString();
        item.sellingPrice = priceItem->data(Qt::UserRole + 1).toDouble();
        item.quantity = qty;

        // Get cost price from DB
        Product p = Database::instance().getProductById(item.productId);
        item.costPrice = p.costPrice;
        item.barcode = p.barcode;

        t.items.append(item);
    }

    if (t.items.isEmpty()) {
        QMessageBox::warning(this, "Error", "No valid items to save.");
        return;
    }

    if (!Database::instance().createTransaction(t)) {
        QMessageBox::critical(this, "Error", "Failed to save transaction:\n" + Database::instance().lastError());
        return;
    }

    double total = computeTotal();
    m_queueTable->setRowCount(0);
    updateTotal();
    loadProducts(m_searchEdit->text());

    QMessageBox::information(this, "Success",
                             QString("Transaction saved successfully!\nTotal: %1").arg(formatCurrency(total)));
}
