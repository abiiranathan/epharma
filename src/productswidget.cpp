#include "productswidget.hpp"
#include <QDate>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QTextStream>
#include <QVBoxLayout>
#include "database.hpp"

// =================== Product Dialog ===================

ProductDialog::ProductDialog(QWidget* parent, const Product& product) : QDialog(parent) {
    m_isEdit = product.id > 0;
    m_productId = product.id;
    setWindowTitle(m_isEdit ? "Edit Product" : "Add New Product");
    setMinimumWidth(420);

    auto* layout = new QVBoxLayout(this);

    auto* title = new QLabel(m_isEdit ? "✏️  Edit Product" : "➕  Add New Product");
    title->setStyleSheet("font-size: 16px; font-weight: 700; color: #1e3a5f; margin-bottom: 8px;");
    layout->addWidget(title);

    auto* form = new QFormLayout;
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignRight);

    m_genericName = new QLineEdit(product.genericName);
    m_genericName->setPlaceholderText("e.g. Tabs Paracetamol 500mg");
    form->addRow("Generic Name*:", m_genericName);

    m_brandName = new QLineEdit(product.brandName);
    m_brandName->setPlaceholderText("e.g. Panadol");
    form->addRow("Brand Name:", m_brandName);

    m_quantity = new QSpinBox;
    m_quantity->setRange(0, 999999);
    m_quantity->setValue(product.quantity);
    form->addRow("Quantity:", m_quantity);

    m_costPrice = new QDoubleSpinBox;
    m_costPrice->setRange(0, 9999999);
    m_costPrice->setDecimals(2);
    m_costPrice->setValue(product.costPrice);
    m_costPrice->setPrefix("UGX ");
    form->addRow("Cost Price:", m_costPrice);

    m_sellingPrice = new QDoubleSpinBox;
    m_sellingPrice->setRange(0, 9999999);
    m_sellingPrice->setDecimals(2);
    m_sellingPrice->setValue(product.sellingPrice);
    m_sellingPrice->setPrefix("UGX ");
    form->addRow("Selling Price:", m_sellingPrice);

    m_barcode = new QLineEdit(product.barcode);
    m_barcode->setPlaceholderText("Scan or enter barcode");
    form->addRow("Barcode:", m_barcode);

    // Expiry dates as comma-separated
    QStringList dateStrings;
    for (const auto& d : product.expiryDates) {
        dateStrings << d.toString("yyyy-MM-dd");
    }
    m_expiryDates = new QLineEdit(dateStrings.join(", "));
    m_expiryDates->setPlaceholderText("yyyy-MM-dd, yyyy-MM-dd");
    form->addRow("Expiry Dates:", m_expiryDates);

    auto* hint = new QLabel("Separate multiple expiry dates with commas");
    hint->setStyleSheet("color: #a0aec0; font-size: 11px;");
    form->addRow("", hint);

    layout->addLayout(form);
    layout->addSpacing(8);

    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    btnBox->button(QDialogButtonBox::Ok)->setText(m_isEdit ? "Update" : "Add Product");
    btnBox->button(QDialogButtonBox::Ok)->setObjectName("successBtn");
    connect(btnBox, &QDialogButtonBox::accepted, this, [this] {
        if (m_genericName->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Validation", "Generic name is required.");
            return;
        }
        accept();
    });
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

Product ProductDialog::getProduct() const {
    Product p;
    p.id = m_productId;
    p.genericName = m_genericName->text().trimmed();
    p.brandName = m_brandName->text().trimmed();
    p.quantity = m_quantity->value();
    p.costPrice = m_costPrice->value();
    p.sellingPrice = m_sellingPrice->value();
    p.barcode = m_barcode->text().trimmed();

    for (const QString& ds : m_expiryDates->text().split(",", Qt::SkipEmptyParts)) {
        QDate d = QDate::fromString(ds.trimmed(), "yyyy-MM-dd");
        if (d.isValid()) {
            p.expiryDates.append(d);
        }
    }
    return p;
}

// =================== Products Widget ===================

ProductsWidget::ProductsWidget(const User& user, QWidget* parent) : QWidget(parent), m_currentUser(user) {
    setupUi();
    refresh();
}

void ProductsWidget::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    // Header
    auto* headerRow = new QHBoxLayout;
    auto* title = new QLabel("📦  Inventory Management");
    title->setObjectName("pageTitle");
    headerRow->addWidget(title);
    headerRow->addStretch();

    m_countLabel = new QLabel;
    m_countLabel->setStyleSheet("color: #4a5568; font-size: 13px;");
    headerRow->addWidget(m_countLabel);
    root->addLayout(headerRow);

    // Toolbar
    auto* toolbar = new QHBoxLayout;
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("🔍  Search by name or brand...");
    m_searchEdit->setObjectName("searchBar");
    m_searchEdit->setFixedHeight(34);
    m_searchEdit->setMaximumWidth(350);

    auto* addBtn = new QPushButton("➕  Add Product");
    addBtn->setObjectName("successBtn");
    addBtn->setFixedHeight(34);

    auto* importBtn = new QPushButton("📥  Import CSV");
    importBtn->setFixedHeight(34);

    toolbar->addWidget(m_searchEdit);
    toolbar->addStretch();
    toolbar->addWidget(addBtn);
    toolbar->addWidget(importBtn);
    root->addLayout(toolbar);

    // Table
    m_table = new QTableWidget;
    m_table->setColumnCount(9);
    m_table->setHorizontalHeaderLabels(
        {"ID", "Generic Name", "Brand", "Qty", "Cost Price", "Selling Price", "Barcode", "Expiry Dates", "Actions"});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setColumnWidth(0, 45);   // for ID
    m_table->setColumnWidth(2, 130);  // brand name
    m_table->setColumnWidth(3, 60);   // for quantity
    m_table->setColumnWidth(4, 100);  // for cost price
    m_table->setColumnWidth(5, 120);  // for selling price
    m_table->setColumnWidth(6, 150);  // barcode
    m_table->setColumnWidth(7, 200);  // for expiry dates
    m_table->setColumnWidth(8, 200);  // for action buttons
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    root->addWidget(m_table);

    // Pagination
    auto* pageRow = new QHBoxLayout;
    m_prevBtn = new QPushButton("← Prev");
    m_prevBtn->setObjectName("secondaryBtn");
    m_prevBtn->setFixedHeight(30);
    m_pageLabel = new QLabel("Page 1");
    m_pageLabel->setAlignment(Qt::AlignCenter);
    m_pageLabel->setStyleSheet("color: #4a5568; font-weight: 500;");
    m_nextBtn = new QPushButton("Next →");
    m_nextBtn->setObjectName("secondaryBtn");
    m_nextBtn->setFixedHeight(30);
    pageRow->addWidget(m_prevBtn);
    pageRow->addStretch();
    pageRow->addWidget(m_pageLabel);
    pageRow->addStretch();
    pageRow->addWidget(m_nextBtn);
    root->addLayout(pageRow);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &ProductsWidget::onSearch);
    connect(addBtn, &QPushButton::clicked, this, &ProductsWidget::onAdd);
    connect(importBtn, &QPushButton::clicked, this, &ProductsWidget::onImport);
    connect(m_prevBtn, &QPushButton::clicked, this, &ProductsWidget::onPrevPage);
    connect(m_nextBtn, &QPushButton::clicked, this, &ProductsWidget::onNextPage);
}

void ProductsWidget::refresh() {
    m_totalCount = Database::instance().countProducts();
    loadPage();
}

void ProductsWidget::onSearch(const QString& text) {
    m_searchFilter = text;
    m_page = 1;
    loadPage();
}

void ProductsWidget::loadPage() {
    int offset = (m_page - 1) * m_pageSize;
    QList<Product> products = Database::instance().listProducts(m_searchFilter, m_pageSize, offset);

    m_table->setRowCount(0);
    m_table->setRowCount(static_cast<int>(products.size()));
    for (int i = 0; i < products.size(); ++i) {
        setRow(i, products[i]);
    }

    int totalPages = qMax(1, (m_totalCount + m_pageSize - 1) / m_pageSize);
    m_pageLabel->setText(QString("Page %1 / %2  (%3 items)").arg(m_page).arg(totalPages).arg(m_totalCount));
    m_prevBtn->setEnabled(m_page > 1);
    m_nextBtn->setEnabled(m_page < totalPages);
    m_countLabel->setText(QString("Total: %1 products").arg(m_totalCount));
}

void ProductsWidget::setRow(int row, const Product& p) {
    auto center = [](const QString& s) {
        auto* item = new QTableWidgetItem(s);
        item->setTextAlignment(Qt::AlignCenter);
        item->setData(Qt::UserRole, 0);
        return item;
    };

    auto* idItem = new QTableWidgetItem(QString::number(p.id));
    idItem->setData(Qt::UserRole, p.id);
    idItem->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(row, 0, idItem);
    m_table->setItem(row, 1, new QTableWidgetItem(p.genericName));
    m_table->setItem(row, 2, new QTableWidgetItem(p.brandName));

    auto* qtyItem = center(QString::number(p.quantity));
    if (p.quantity == 0) {
        qtyItem->setForeground(QColor("#e53e3e"));
    } else if (p.quantity < 10) {
        qtyItem->setForeground(QColor("#d97706"));
    } else {
        qtyItem->setForeground(QColor("#2f855a"));
    }
    m_table->setItem(row, 3, qtyItem);

    m_table->setItem(row, 4, new QTableWidgetItem(QString::number(p.costPrice, 'f', 2)));
    m_table->setItem(row, 5, new QTableWidgetItem(QString::number(p.sellingPrice, 'f', 2)));
    m_table->setItem(row, 6, new QTableWidgetItem(p.barcode));

    QStringList dates;
    QDate today = QDate::currentDate();
    for (const auto& d : p.expiryDates) {
        QString ds = d.toString("MMM yyyy");
        if (d < today) {
            ds += " ⚠️";
        }
        dates << ds;
    }
    m_table->setItem(row, 7, new QTableWidgetItem(dates.join(" | ")));

    // Actions
    auto* actWidget = new QWidget;
    auto* actLayout = new QHBoxLayout(actWidget);
    actLayout->setContentsMargins(4, 2, 4, 2);
    actLayout->setSpacing(4);

    auto* editBtn = new QPushButton("Edit");
    editBtn->setFixedHeight(26);
    editBtn->setFixedWidth(46);
    editBtn->setStyleSheet(
        "QPushButton { background-color: #3a7bd5; color: white; border: none; "
        "border-radius: 3px; font-size: 12px; }"
        "QPushButton:hover { background-color: #2d6bc4; }");
    auto* delBtn = new QPushButton("Del");
    delBtn->setObjectName("dangerBtn");
    delBtn->setFixedHeight(26);
    delBtn->setFixedWidth(40);
    delBtn->setStyleSheet(
        "QPushButton { background-color: #e53e3e; color: white; border: none; "
        "border-radius: 3px; font-size: 12px; }"
        "QPushButton:hover { background-color: #c53030; }");

    int pid = p.id;
    connect(editBtn, &QPushButton::clicked, this, [this, pid] {
        Product p = Database::instance().getProductById(pid);
        ProductDialog dlg(this, p);
        if (dlg.exec() == QDialog::Accepted) {
            Product updated = dlg.getProduct();
            if (!Database::instance().updateProduct(updated)) {
                QMessageBox::critical(this, "Error", Database::instance().lastError());
            } else {
                refresh();
            }
        }
    });
    connect(delBtn, &QPushButton::clicked, this, [this, pid] {
        auto ret = QMessageBox::question(this, "Delete Product", "Are you sure you want to delete this product?",
                                         QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            if (!Database::instance().deleteProduct(pid)) {
                QMessageBox::critical(this, "Error", Database::instance().lastError());
            } else {
                refresh();
            }
        }
    });

    actLayout->addWidget(editBtn);
    actLayout->addWidget(delBtn);
    m_table->setCellWidget(row, 8, actWidget);
}

int ProductsWidget::selectedId() const {
    int row = m_table->currentRow();
    if (row < 0) {
        return -1;
    }
    auto* item = m_table->item(row, 0);
    if (!item) {
        return -1;
    }
    return item->data(Qt::UserRole).toInt();
}

void ProductsWidget::onAdd() {
    ProductDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        Product p = dlg.getProduct();
        if (!Database::instance().createProduct(p)) {
            QMessageBox::critical(this, "Error", Database::instance().lastError());
        } else {
            refresh();
        }
    }
}

void ProductsWidget::onEdit() {
    int id = selectedId();
    if (id < 0) {
        QMessageBox::information(this, "Select Product", "Please select a product to edit.");
        return;
    }
    Product p = Database::instance().getProductById(id);
    ProductDialog dlg(this, p);
    if (dlg.exec() == QDialog::Accepted) {
        Product updated = dlg.getProduct();
        if (!Database::instance().updateProduct(updated)) {
            QMessageBox::critical(this, "Error", Database::instance().lastError());
        } else {
            refresh();
        }
    }
}

void ProductsWidget::onDelete() {
    int id = selectedId();
    if (id < 0) {
        QMessageBox::information(this, "Select Product", "Please select a product to delete.");
        return;
    }
    auto ret = QMessageBox::question(this, "Delete Product", "Are you sure you want to delete this product?",
                                     QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        if (!Database::instance().deleteProduct(id)) {
            QMessageBox::critical(this, "Error", Database::instance().lastError());
        } else {
            refresh();
        }
    }
}

void ProductsWidget::onImport() {
    QString path =
        QFileDialog::getOpenFileName(this, "Import Products CSV", QString(), "CSV Files (*.csv);;All Files (*)");
    if (path.isEmpty()) {
        return;
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot open file: " + f.errorString());
        return;
    }

    QTextStream stream(&f);
    QList<Product> products;
    bool firstLine = true;

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (firstLine) {
            firstLine = false;
            continue;
        }  // skip header
        if (line.isEmpty()) {
            continue;
        }

        QStringList parts = line.split(",");
        if (parts.size() < 7) {
            continue;
        }

        Product p;
        p.genericName = parts[0].trimmed();
        p.brandName = parts[1].trimmed();
        p.quantity = parts[2].trimmed().toInt();
        // parts[3] = expiry dates colon-separated
        for (const QString& ds : parts[3].split(";", Qt::SkipEmptyParts)) {
            QDate d = QDate::fromString(ds.trimmed(), "yyyy-MM-dd");
            if (d.isValid()) {
                p.expiryDates.append(d);
            }
        }
        p.costPrice = parts[4].trimmed().replace(",", "").toDouble();
        p.sellingPrice = parts[5].trimmed().replace(",", "").toDouble();
        p.barcode = parts[6].trimmed();
        products.append(p);
    }
    f.close();

    if (products.isEmpty()) {
        QMessageBox::warning(this, "Import", "No valid products found in file.");
        return;
    }

    if (!Database::instance().importProducts(products)) {
        QMessageBox::critical(this, "Import Error", Database::instance().lastError());
    } else {
        QMessageBox::information(this, "Import Success",
                                 QString("Successfully imported %1 products.").arg(products.size()));
        refresh();
    }
}

void ProductsWidget::onPrevPage() {
    if (m_page > 1) {
        m_page--;
        loadPage();
    }
}

void ProductsWidget::onNextPage() {
    int totalPages = qMax(1, (m_totalCount + m_pageSize - 1) / m_pageSize);
    if (m_page < totalPages) {
        m_page++;
        loadPage();
    }
}
