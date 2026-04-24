#include "invoiceswidget.hpp"
#include <QCompleter>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QStringListModel>
#include <QVBoxLayout>
#include "database.hpp"

// =================== InvoiceDialog ===================

InvoiceDialog::InvoiceDialog(QWidget* parent, const Invoice& inv)
    : QDialog(parent), m_isEdit(inv.id > 0), m_invoiceId(inv.id) {
    setWindowTitle(m_isEdit ? "Edit Invoice" : "New Invoice");
    setMinimumWidth(400);

    auto* layout = new QVBoxLayout(this);
    auto* title = new QLabel(m_isEdit ? "✏️  Edit Invoice" : "📄  New Invoice");
    title->setStyleSheet("font-size: 16px; font-weight: 700; color: #1e3a5f; margin-bottom: 8px;");
    layout->addWidget(title);

    auto* form = new QFormLayout;
    form->setSpacing(10);

    m_invoiceNumber = new QLineEdit(inv.invoiceNumber);
    m_invoiceNumber->setPlaceholderText("e.g. INV-2024-001");
    form->addRow("Invoice Number*:", m_invoiceNumber);

    m_purchaseDate = new QDateEdit(inv.purchaseDate.isValid() ? inv.purchaseDate : QDate::currentDate());
    m_purchaseDate->setCalendarPopup(true);
    m_purchaseDate->setDisplayFormat("yyyy-MM-dd");
    form->addRow("Purchase Date*:", m_purchaseDate);

    m_invoiceTotal = new QDoubleSpinBox;
    m_invoiceTotal->setRange(0, 999999999);
    m_invoiceTotal->setDecimals(2);
    m_invoiceTotal->setPrefix("UGX ");
    m_invoiceTotal->setValue(inv.invoiceTotal);
    form->addRow("Invoice Total*:", m_invoiceTotal);

    m_amountPaid = new QDoubleSpinBox;
    m_amountPaid->setRange(0, 999999999);
    m_amountPaid->setDecimals(2);
    m_amountPaid->setPrefix("UGX ");
    m_amountPaid->setValue(inv.amountPaid);
    form->addRow("Amount Paid*:", m_amountPaid);

    m_supplier = new QLineEdit(inv.supplier);
    m_supplier->setPlaceholderText("Supplier name");
    form->addRow("Supplier*:", m_supplier);

    layout->addLayout(form);

    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    btnBox->button(QDialogButtonBox::Ok)->setText(m_isEdit ? "Update" : "Create Invoice");
    btnBox->button(QDialogButtonBox::Ok)->setObjectName("successBtn");
    connect(btnBox, &QDialogButtonBox::accepted, this, [this] {
        if (m_invoiceNumber->text().trimmed().isEmpty() || m_supplier->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Validation", "Invoice number and supplier are required.");
            return;
        }
        accept();
    });
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

Invoice InvoiceDialog::getInvoice() const {
    Invoice inv;
    inv.id = m_invoiceId;
    inv.invoiceNumber = m_invoiceNumber->text().trimmed();
    inv.purchaseDate = m_purchaseDate->date();
    inv.invoiceTotal = m_invoiceTotal->value();
    inv.amountPaid = m_amountPaid->value();
    inv.supplier = m_supplier->text().trimmed();
    return inv;
}

// =================== StockInDialog ===================

StockInDialog::StockInDialog(int invoiceId, QWidget* parent) : QDialog(parent), m_invoiceId(invoiceId) {
    setWindowTitle("Add Product to Invoice");
    setMinimumWidth(720);

    auto* layout = new QVBoxLayout(this);
    auto* title = new QLabel("📦  Add Stock Item");
    title->setStyleSheet("font-size: 16px; font-weight: 700; color: #1e3a5f; margin-bottom: 8px;");
    layout->addWidget(title);

    auto* form = new QFormLayout;
    form->setSpacing(10);

    m_productSearch = new QLineEdit;
    m_productSearch->setPlaceholderText("Type product name to search...");
    m_productLabel = new QLabel("No product selected");
    m_productLabel->setStyleSheet("color: #718096; font-size: 12px;");
    form->addRow("Product Name*:", m_productSearch);
    form->addRow("", m_productLabel);

    m_quantity = new QSpinBox;
    m_quantity->setRange(1, 999999);
    m_quantity->setValue(1);
    form->addRow("Quantity*:", m_quantity);

    m_costPrice = new QDoubleSpinBox;
    m_costPrice->setRange(0, 9999999);
    m_costPrice->setDecimals(2);
    m_costPrice->setPrefix("UGX ");
    form->addRow("Cost Price (Rate)*:", m_costPrice);

    m_expiryDate = new QDateEdit(QDate::currentDate().addYears(2));
    m_expiryDate->setCalendarPopup(true);
    m_expiryDate->setDisplayFormat("yyyy-MM-dd");
    form->addRow("Expiry Date:", m_expiryDate);

    m_comment = new QLineEdit;
    m_comment->setPlaceholderText("Optional comment");
    form->addRow("Comment:", m_comment);

    layout->addLayout(form);

    // Search timer for autocomplete
    connect(m_productSearch, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (text.length() < 2) {
            return;
        }
        auto products = Database::instance().searchProducts(text, 10);
        if (!products.isEmpty()) {
            m_selectedProductId = products[0].id;
            m_productLabel->setText(QString("✓ %1 (ID: %2, Stock: %3)")
                                        .arg(products[0].displayName())
                                        .arg(products[0].id)
                                        .arg(products[0].quantity));
            m_productLabel->setStyleSheet("color: #2f855a; font-size: 12px; font-weight: 500;");
            m_costPrice->setValue(products[0].costPrice);
        } else {
            m_selectedProductId = 0;
            m_productLabel->setText("No product found");
            m_productLabel->setStyleSheet("color: #e53e3e; font-size: 12px;");
        }
    });

    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    btnBox->button(QDialogButtonBox::Ok)->setText("Add to Invoice");
    btnBox->button(QDialogButtonBox::Ok)->setObjectName("successBtn");
    connect(btnBox, &QDialogButtonBox::accepted, this, [this] {
        if (m_selectedProductId == 0) {
            QMessageBox::warning(this, "Validation", "Please select a valid product.");
            return;
        }
        if (m_quantity->value() <= 0 || m_costPrice->value() <= 0) {
            QMessageBox::warning(this, "Validation", "Quantity and cost price must be positive.");
            return;
        }
        accept();
    });
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

StockInItem StockInDialog::getStockIn() const {
    StockInItem si;
    si.productId = m_selectedProductId;
    si.invoiceId = m_invoiceId;
    si.quantity = m_quantity->value();
    si.costPrice = m_costPrice->value();
    si.expiryDate = m_expiryDate->date();
    si.comment = m_comment->text().trimmed();
    return si;
}

// =================== InvoiceDetailWidget ===================

InvoiceDetailWidget::InvoiceDetailWidget(const Invoice& inv, const User& user, QWidget* parent)
    : QWidget(parent), m_invoice(inv), m_user(user) {
    setupUi();
    refreshItems();
}

void InvoiceDetailWidget::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    // Header
    auto* headerRow = new QHBoxLayout;
    auto* backBtn = new QPushButton("← Back to Invoices");
    backBtn->setObjectName("secondaryBtn");
    backBtn->setFixedHeight(32);
    connect(backBtn, &QPushButton::clicked, this, &InvoiceDetailWidget::backRequested);

    auto* title = new QLabel(QString("Invoice: %1").arg(m_invoice.invoiceNumber));
    title->setObjectName("pageTitle");
    headerRow->addWidget(backBtn);
    headerRow->addSpacing(12);
    headerRow->addWidget(title);
    headerRow->addStretch();
    root->addLayout(headerRow);

    // Summary card
    auto* summary = new QGroupBox("Invoice Details");
    auto* summaryLayout = new QHBoxLayout(summary);

    auto addInfo = [&](const QString& label, const QString& val, const QString& color = "#1e3a5f") {
        auto* col = new QVBoxLayout;
        auto* lbl = new QLabel(label);
        lbl->setStyleSheet("color: #718096; font-size: 11px; font-weight: 600;");
        auto* val_lbl = new QLabel(val);
        val_lbl->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: 600;").arg(color));
        col->addWidget(lbl);
        col->addWidget(val_lbl);
        summaryLayout->addLayout(col);
        summaryLayout->addSpacing(20);
    };

    addInfo("SUPPLIER", m_invoice.supplier);
    addInfo("DATE", m_invoice.purchaseDate.toString("dd MMM yyyy"));
    addInfo("TOTAL", QString("UGX %L1").arg(m_invoice.invoiceTotal, 0, 'f', 2));
    addInfo("PAID", QString("UGX %L1").arg(m_invoice.amountPaid, 0, 'f', 2));
    double bal = m_invoice.invoiceTotal - m_invoice.amountPaid;
    addInfo("BALANCE", QString("UGX %L1").arg(bal, 0, 'f', 2), bal > 0 ? "#e53e3e" : "#2f855a");

    summaryLayout->addStretch();
    root->addWidget(summary);

    // Add stock in button
    auto* addBtn = new QPushButton("➕  Add Product");
    addBtn->setObjectName("successBtn");
    addBtn->setFixedHeight(34);
    addBtn->setFixedWidth(150);
    root->addWidget(addBtn);
    connect(addBtn, &QPushButton::clicked, this, &InvoiceDetailWidget::onAddStockIn);

    // Items table
    auto* itemsLabel = new QLabel("Stock Items");
    itemsLabel->setStyleSheet("font-size: 14px; font-weight: 600; color: #2d3748;");
    root->addWidget(itemsLabel);

    m_itemsTable = new QTableWidget;
    m_itemsTable->setColumnCount(7);
    m_itemsTable->setHorizontalHeaderLabels({"Product Name", "Brand", "Qty", "Rate", "Total", "Expiry", "Action"});
    m_itemsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_itemsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_itemsTable->setAlternatingRowColors(true);
    m_itemsTable->verticalHeader()->setVisible(false);
    m_itemsTable->setShowGrid(false);

    // Set ALL columns to stretch first — distributes space proportionally
    m_itemsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Then pin the narrow columns to fixed widths so they don't shrink/grow
    m_itemsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);    // Qty
    m_itemsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);  // Rate
    m_itemsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);  // Total
    m_itemsTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);    // Expiry
    m_itemsTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);    // Action

    m_itemsTable->setColumnWidth(1, 150);  // Brand
    m_itemsTable->setColumnWidth(2, 60);   // Qty
    m_itemsTable->setColumnWidth(3, 130);  // Rate
    m_itemsTable->setColumnWidth(4, 130);  // Total
    m_itemsTable->setColumnWidth(5, 110);  // Expiry
    m_itemsTable->setColumnWidth(6, 70);   // Action

    // Columns 0 (Product Name) and 1 (Brand) will now stretch to fill the remaining space
    root->addWidget(m_itemsTable);
}

void InvoiceDetailWidget::refreshItems() {
    QList<StockInItem> items = Database::instance().getStockInByInvoice(m_invoice.id);
    m_itemsTable->setRowCount(0);
    m_itemsTable->setRowCount(static_cast<int>(items.size()));
    double grandTotal = 0;

    for (int i = 0; i < items.size(); ++i) {
        const StockInItem& si = items[i];
        m_itemsTable->setItem(i, 0, new QTableWidgetItem(si.genericName));
        m_itemsTable->setItem(i, 1, new QTableWidgetItem(si.brandName));

        auto* qtyItem = new QTableWidgetItem(QString::number(si.quantity));
        qtyItem->setTextAlignment(Qt::AlignCenter);
        m_itemsTable->setItem(i, 2, qtyItem);

        m_itemsTable->setItem(i, 3, new QTableWidgetItem(QString("UGX %1").arg(si.costPrice, 0, 'f', 2)));

        double total = si.quantity * si.costPrice;
        grandTotal += total;
        m_itemsTable->setItem(i, 4, new QTableWidgetItem(QString("UGX %1").arg(total, 0, 'f', 2)));

        m_itemsTable->setItem(
            i, 5, new QTableWidgetItem(si.expiryDate.isValid() ? si.expiryDate.toString("MMM yyyy") : "N/A"));

        auto* delBtn = new QPushButton("Del");
        delBtn->setFixedHeight(26);
        delBtn->setStyleSheet(
            "QPushButton { background-color: #e53e3e; color: white; border: none; "
            "border-radius: 3px; font-size: 12px; padding: 0 8px; }"
            "QPushButton:hover { background-color: #c53030; }");
        int sid = si.id;
        connect(delBtn, &QPushButton::clicked, this, [this, sid] { onDeleteStockIn(sid); });
        m_itemsTable->setCellWidget(i, 6, delBtn);
    }

    // TODO: update invoice total in database if grandTotal differs from m_invoice.invoiceTotal
    (void)grandTotal;
}

void InvoiceDetailWidget::onAddStockIn() {
    StockInDialog dlg(m_invoice.id, this);
    if (dlg.exec() == QDialog::Accepted) {
        StockInItem si = dlg.getStockIn();
        if (!Database::instance().addStockIn(si)) {
            QMessageBox::critical(this, "Error", Database::instance().lastError());
        } else {
            refreshItems();
        }
    }
}

void InvoiceDetailWidget::onDeleteStockIn(int id) {
    auto ret = QMessageBox::question(this, "Delete Stock Item",
                                     "Remove this item from the invoice? This will also reduce product stock.",
                                     QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        if (!Database::instance().deleteStockIn(id)) {
            QMessageBox::critical(this, "Error", Database::instance().lastError());
        } else {
            refreshItems();
        }
    }
}

// =================== InvoicesWidget ===================

InvoicesWidget::InvoicesWidget(const User& user, QWidget* parent) : QWidget(parent), m_currentUser(user) { setupUi(); }

void InvoicesWidget::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    m_stack = new QStackedWidget;

    // Page 0: list
    auto* listPage = new QWidget;
    auto* listLayout = new QVBoxLayout(listPage);
    listLayout->setContentsMargins(16, 16, 16, 16);
    listLayout->setSpacing(12);

    auto* headerRow = new QHBoxLayout;
    auto* title = new QLabel("📄  Invoices");
    title->setObjectName("pageTitle");
    headerRow->addWidget(title);
    headerRow->addStretch();
    listLayout->addLayout(headerRow);

    auto* toolbar = new QHBoxLayout;
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("🔍  Search by invoice number...");
    m_searchEdit->setObjectName("searchBar");
    m_searchEdit->setFixedHeight(34);
    m_searchEdit->setMaximumWidth(300);

    auto* addBtn = new QPushButton("➕  New Invoice");
    addBtn->setObjectName("successBtn");
    addBtn->setFixedHeight(34);
    toolbar->addWidget(m_searchEdit);
    toolbar->addStretch();
    toolbar->addWidget(addBtn);
    listLayout->addLayout(toolbar);

    m_table = new QTableWidget;
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels(
        {"Date", "Invoice #", "Supplier", "Total", "Paid", "Balance", "Created", "Actions"});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);

    for (int i = 0; i < m_table->columnCount(); ++i) {
        m_table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    }

    m_table->setColumnWidth(0, 100);
    m_table->setColumnWidth(1, 150);
    m_table->setColumnWidth(3, 120);
    m_table->setColumnWidth(4, 120);
    m_table->setColumnWidth(5, 120);
    m_table->setColumnWidth(6, 140);
    m_table->setColumnWidth(7, 150);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    listLayout->addWidget(m_table);

    m_stack->addWidget(listPage);  // index 0
    root->addWidget(m_stack);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &InvoicesWidget::onSearch);
    connect(addBtn, &QPushButton::clicked, this, &InvoicesWidget::onAdd);
}

void InvoicesWidget::refresh() {
    m_stack->setCurrentIndex(0);
    loadData();
}

void InvoicesWidget::onSearch(const QString& text) {
    Q_UNUSED(text)
    loadData();
}

void InvoicesWidget::loadData() {
    QList<Invoice> invoices = Database::instance().listInvoices(100, 0);
    QString filter = m_searchEdit->text().trimmed().toLower();

    m_table->setRowCount(0);
    int row = 0;
    for (const auto& inv : invoices) {
        if (!filter.isEmpty() && !inv.invoiceNumber.toLower().contains(filter) &&
            !inv.supplier.toLower().contains(filter)) {
            continue;
        }
        m_table->insertRow(row);
        setRow(row, inv);
        row++;
    }
}

void InvoicesWidget::setRow(int row, const Invoice& inv) {
    m_table->setItem(row, 0, new QTableWidgetItem(inv.purchaseDate.toString("dd MMM yyyy")));

    auto* numItem = new QTableWidgetItem(inv.invoiceNumber);
    numItem->setForeground(QColor("#2b6cb0"));
    numItem->setData(Qt::UserRole, inv.id);
    m_table->setItem(row, 1, numItem);

    m_table->setItem(row, 2, new QTableWidgetItem(inv.supplier));
    m_table->setItem(row, 3, new QTableWidgetItem(QString("UGX %1").arg(inv.invoiceTotal, 0, 'f', 2)));
    m_table->setItem(row, 4, new QTableWidgetItem(QString("UGX %1").arg(inv.amountPaid, 0, 'f', 2)));

    double bal = inv.invoiceTotal - inv.amountPaid;
    auto* balItem = new QTableWidgetItem(QString("UGX %1").arg(bal, 0, 'f', 2));
    if (bal > 0) {
        balItem->setForeground(QColor("#e53e3e"));
    } else {
        balItem->setForeground(QColor("#2f855a"));
    }
    m_table->setItem(row, 5, balItem);

    m_table->setItem(row, 6, new QTableWidgetItem(inv.createdAt.toString("dd MMM yyyy hh:mm")));

    // Actions
    auto* actWidget = new QWidget;
    auto* actLayout = new QHBoxLayout(actWidget);
    actLayout->setContentsMargins(4, 2, 4, 2);
    actLayout->setSpacing(4);

    auto* viewBtn = new QPushButton("View");
    viewBtn->setFixedHeight(26);
    viewBtn->setStyleSheet(
        "QPushButton { background-color: #3a7bd5; color: white; border: none; "
        "border-radius: 3px; font-size: 12px; padding: 0 8px; }"
        "QPushButton:hover { background-color: #2d6bc4; }");
    auto* editBtn = new QPushButton("Edit");
    editBtn->setFixedHeight(26);
    editBtn->setStyleSheet(
        "QPushButton { background-color: #718096; color: white; border: none; "
        "border-radius: 3px; font-size: 12px; padding: 0 8px; }"
        "QPushButton:hover { background-color: #4a5568; }");
    auto* delBtn = new QPushButton("Del");
    delBtn->setFixedHeight(26);
    delBtn->setStyleSheet(
        "QPushButton { background-color: #e53e3e; color: white; border: none; "
        "border-radius: 3px; font-size: 12px; padding: 0 8px; }"
        "QPushButton:hover { background-color: #c53030; }");

    int iid = inv.id;
    connect(viewBtn, &QPushButton::clicked, this, [this, iid] { onViewDetails(iid); });
    connect(editBtn, &QPushButton::clicked, this, [this, iid] {
        Invoice inv = Database::instance().getInvoiceById(iid);
        InvoiceDialog dlg(this, inv);
        if (dlg.exec() == QDialog::Accepted) {
            Invoice updated = dlg.getInvoice();
            updated.userId = m_currentUser.id;
            if (!Database::instance().updateInvoice(updated)) {
                QMessageBox::critical(this, "Error", Database::instance().lastError());
            } else {
                loadData();
            }
        }
    });
    connect(delBtn, &QPushButton::clicked, this, [this, iid] {
        auto ret = QMessageBox::question(this, "Delete Invoice", "Delete this invoice and all its stock items?",
                                         QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            if (!Database::instance().deleteInvoice(iid)) {
                QMessageBox::critical(this, "Error", Database::instance().lastError());
            } else {
                loadData();
            }
        }
    });

    actLayout->addWidget(viewBtn);
    actLayout->addWidget(editBtn);
    actLayout->addWidget(delBtn);
    m_table->setCellWidget(row, 7, actWidget);
}

void InvoicesWidget::onAdd() {
    InvoiceDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        Invoice inv = dlg.getInvoice();
        inv.userId = m_currentUser.id;
        if (!Database::instance().createInvoice(inv)) {
            QMessageBox::critical(this, "Error", Database::instance().lastError());
        } else {
            loadData();
            // Switch to detail view
            onViewDetails(inv.id);
        }
    }
}

void InvoicesWidget::onEdit() {}
void InvoicesWidget::onDelete() {}

void InvoicesWidget::onViewDetails(int id) {
    Invoice inv = Database::instance().getInvoiceById(id);
    if (inv.id == 0) {
        return;
    }

    // Remove old detail widget if any
    if (m_detailWidget) {
        m_stack->removeWidget(m_detailWidget);
        delete m_detailWidget;
        m_detailWidget = nullptr;
    }

    m_detailWidget = new InvoiceDetailWidget(inv, m_currentUser);
    connect(m_detailWidget, &InvoiceDetailWidget::backRequested, this, [this] {
        m_stack->setCurrentIndex(0);
        loadData();
    });
    m_stack->addWidget(m_detailWidget);
    m_stack->setCurrentWidget(m_detailWidget);
}
