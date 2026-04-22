#include "database.hpp"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QVariant>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

Database& Database::instance() {
    static Database db;
    return db;
}

bool Database::open(const QString& path) {
    m_db = QSqlDatabase::addDatabase("QSQLITE", "epharmacy");
    m_db.setDatabaseName(path);
    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        return false;
    }

    // Enable WAL and foreign keys
    QSqlQuery q(m_db);
    q.exec("PRAGMA journal_mode=WAL");
    q.exec("PRAGMA foreign_keys=ON");
    q.exec("PRAGMA synchronous=NORMAL");

    return initSchema();
}

void Database::close() {
    QString connName = m_db.connectionName();
    m_db.close();

    m_db = QSqlDatabase();  // release the member's reference
    QSqlDatabase::removeDatabase(connName);
}

bool Database::isOpen() const { return m_db.isOpen(); }

QString Database::lastError() const { return m_lastError; }

static QString hashPassword(const QString& pw) {
    return QCryptographicHash::hash(pw.toUtf8(), QCryptographicHash::Sha256).toHex();
}

bool Database::initSchema() {
    QSqlQuery q(m_db);

    // Users
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            password TEXT NOT NULL,
            is_active INTEGER NOT NULL DEFAULT 1,
            is_admin INTEGER NOT NULL DEFAULT 0,
            created_at TEXT NOT NULL DEFAULT (datetime('now'))
        )
    )")) {
        m_lastError = q.lastError().text();
        return false;
    }

    // Products
    // -- NULLs are always considered distinct in SQLite,
    // so we can have multiple products with empty barcode.
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS products (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            generic_name TEXT NOT NULL,
            brand_name TEXT NOT NULL DEFAULT '',
            quantity INTEGER NOT NULL DEFAULT 0,
            cost_price REAL NOT NULL DEFAULT 0.0,
            selling_price REAL NOT NULL DEFAULT 0.0,
            barcode TEXT NULL,
            created_at TEXT NOT NULL DEFAULT (datetime('now')),
            updated_at TEXT NOT NULL DEFAULT (datetime('now')),
            UNIQUE(generic_name, brand_name),
            UNIQUE(barcode) 
        )
    )")) {
        m_lastError = q.lastError().text();
        return false;
    }

    // Product expiry dates (separate table since SQLite has no array type)
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS product_expiry_dates (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            product_id INTEGER NOT NULL,
            expiry_date TEXT NOT NULL,
            FOREIGN KEY (product_id) REFERENCES products(id) ON DELETE CASCADE,
            UNIQUE(product_id, expiry_date)
        )
    )")) {
        m_lastError = q.lastError().text();
        return false;
    }

    // Transactions
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS transactions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            items TEXT NOT NULL,
            created_at TEXT NOT NULL DEFAULT (datetime('now')),
            user_id INTEGER NOT NULL,
            FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE RESTRICT
        )
    )")) {
        m_lastError = q.lastError().text();
        return false;
    }

    // Invoices
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS invoices (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            invoice_number TEXT NOT NULL UNIQUE,
            purchase_date TEXT NOT NULL,
            invoice_total REAL NOT NULL DEFAULT 0.0,
            amount_paid REAL NOT NULL DEFAULT 0.0,
            supplier TEXT NOT NULL DEFAULT '',
            user_id INTEGER NOT NULL,
            created_at TEXT NOT NULL DEFAULT (datetime('now')),
            FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE RESTRICT
        )
    )")) {
        m_lastError = q.lastError().text();
        return false;
    }

    // Stock In
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS stock_in (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            product_id INTEGER NOT NULL,
            invoice_id INTEGER NOT NULL,
            quantity INTEGER NOT NULL DEFAULT 0,
            cost_price REAL NOT NULL DEFAULT 0.0,
            expiry_date TEXT NOT NULL DEFAULT '',
            comment TEXT NOT NULL DEFAULT '',
            created_at TEXT NOT NULL DEFAULT (datetime('now')),
            FOREIGN KEY (product_id) REFERENCES products(id) ON DELETE CASCADE,
            FOREIGN KEY (invoice_id) REFERENCES invoices(id) ON DELETE CASCADE
        )
    )")) {
        m_lastError = q.lastError().text();
        return false;
    }

    // Stock balances (daily snapshot)
    if (!q.exec(R"(
        CREATE TABLE IF NOT EXISTS stock_balances (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            product_id INTEGER NOT NULL,
            opening_quantity INTEGER NOT NULL DEFAULT 0,
            quantity_in INTEGER NOT NULL DEFAULT 0,
            balance_date TEXT NOT NULL DEFAULT (date('now')),
            FOREIGN KEY (product_id) REFERENCES products(id) ON DELETE CASCADE,
            UNIQUE(product_id, balance_date)
        )
    )")) {
        m_lastError = q.lastError().text();
        return false;
    }

    // Create default admin user if no users exist
    q.exec("SELECT COUNT(*) FROM users");
    if (q.next() && q.value(0).toInt() == 0) {
        QSqlQuery ins(m_db);
        ins.prepare("INSERT INTO users (username, password, is_active, is_admin) VALUES (?, ?, 1, 1)");
        ins.addBindValue("admin");
        ins.addBindValue(hashPassword("admin123"));
        if (!ins.exec()) {
            m_lastError = ins.lastError().text();
            qWarning() << "Could not create default admin:" << m_lastError;
        }
    }

    return true;
}

// =================== USERS ===================

bool Database::createUser(const QString& username, const QString& password, bool isAdmin) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO users (username, password, is_active, is_admin) VALUES (?, ?, 1, ?)");
    q.addBindValue(username);
    q.addBindValue(hashPassword(password));
    q.addBindValue(isAdmin ? 1 : 0);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool Database::updateUser(int id, const QString& username, const QString& newPassword, bool updatePassword) {
    QSqlQuery q(m_db);
    if (updatePassword) {
        q.prepare("UPDATE users SET username=?, password=? WHERE id=?");
        q.addBindValue(username);
        q.addBindValue(hashPassword(newPassword));
        q.addBindValue(id);
    } else {
        q.prepare("UPDATE users SET username=? WHERE id=?");
        q.addBindValue(username);
        q.addBindValue(id);
    }
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool Database::deleteUser(int id) {
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM users WHERE id=?");
    q.addBindValue(id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool Database::activateUser(int id) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE users SET is_active=1 WHERE id=?");
    q.addBindValue(id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool Database::deactivateUser(int id) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE users SET is_active=0 WHERE id=?");
    q.addBindValue(id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool Database::promoteUser(int id) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE users SET is_admin=1 WHERE id=?");
    q.addBindValue(id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool Database::demoteUser(int id) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE users SET is_admin=0 WHERE id=?");
    q.addBindValue(id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

User Database::userFromQuery(QSqlQuery& q) {
    User u;
    u.id = q.value("id").toInt();
    u.username = q.value("username").toString();
    u.password = q.value("password").toString();
    u.isActive = q.value("is_active").toInt() != 0;
    u.isAdmin = q.value("is_admin").toInt() != 0;
    u.createdAt = QDateTime::fromString(q.value("created_at").toString(), "yyyy-MM-dd hh:mm:ss");
    return u;
}

QList<User> Database::listUsers() {
    QList<User> users;
    QSqlQuery q("SELECT * FROM users ORDER BY id", m_db);
    while (q.next()) {
        users.append(userFromQuery(q));
    }
    return users;
}

User Database::getUserById(int id) {
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM users WHERE id=?");
    q.addBindValue(id);
    q.exec();
    if (q.next()) {
        return userFromQuery(q);
    }
    return User{};
}

User Database::getUserByUsername(const QString& username) {
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM users WHERE username=? LIMIT 1");
    q.addBindValue(username);
    q.exec();
    if (q.next()) {
        return userFromQuery(q);
    }
    return User{};
}

// =================== PRODUCTS ===================

QList<QDate> Database::getProductExpiry(int productId) {
    QList<QDate> dates;
    QSqlQuery q(m_db);
    q.prepare("SELECT expiry_date FROM product_expiry_dates WHERE product_id=? ORDER BY expiry_date");
    q.addBindValue(productId);
    q.exec();
    while (q.next()) {
        dates.append(QDate::fromString(q.value(0).toString(), Qt::ISODate));
    }
    return dates;
}

bool Database::updateProductExpiry(int productId, const QList<QDate>& dates) {
    QSqlQuery del(m_db);
    del.prepare("DELETE FROM product_expiry_dates WHERE product_id=?");
    del.addBindValue(productId);
    if (!del.exec()) {
        m_lastError = del.lastError().text();
        return false;
    }

    for (const auto& d : dates) {
        QSqlQuery ins(m_db);
        ins.prepare("INSERT OR IGNORE INTO product_expiry_dates (product_id, expiry_date) VALUES (?,?)");
        ins.addBindValue(productId);
        ins.addBindValue(d.toString(Qt::ISODate));
        ins.exec();
    }
    return true;
}

bool Database::addProductExpiry(int productId, const QDate& date) {
    QSqlQuery q(m_db);
    q.prepare("INSERT OR IGNORE INTO product_expiry_dates (product_id, expiry_date) VALUES (?,?)");
    q.addBindValue(productId);
    q.addBindValue(date.toString(Qt::ISODate));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool Database::removeProductExpiry(int productId, const QDate& date) {
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM product_expiry_dates WHERE product_id=? AND expiry_date=?");
    q.addBindValue(productId);
    q.addBindValue(date.toString(Qt::ISODate));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

Product Database::productFromQuery(QSqlQuery& q) {
    Product p;
    p.id = q.value("id").toInt();
    p.genericName = q.value("generic_name").toString();
    p.brandName = q.value("brand_name").toString();
    p.quantity = q.value("quantity").toInt();
    p.costPrice = q.value("cost_price").toDouble();
    p.sellingPrice = q.value("selling_price").toDouble();

    // Handle NULL barcode as empty string
    if (q.value("barcode").isNull()) {
        p.barcode = "";
    } else {
        p.barcode = q.value("barcode").toString();
    }
    p.createdAt = QDateTime::fromString(q.value("created_at").toString(), "yyyy-MM-dd hh:mm:ss");
    p.updatedAt = QDateTime::fromString(q.value("updated_at").toString(), "yyyy-MM-dd hh:mm:ss");
    p.expiryDates = getProductExpiry(p.id);
    return p;
}

bool Database::createProduct(const Product& p) {
    QSqlQuery q(m_db);
    q.prepare(
        R"(INSERT INTO products (generic_name, brand_name, quantity, cost_price, selling_price, barcode, updated_at)
                 VALUES (?, ?, ?, ?, ?, ?, datetime('now')))");
    q.addBindValue(p.genericName);
    q.addBindValue(p.brandName);
    q.addBindValue(p.quantity);
    q.addBindValue(p.costPrice);
    q.addBindValue(p.sellingPrice);
    q.addBindValue(p.barcode.isEmpty() ? QVariant() : QVariant(p.barcode));

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    int newId = q.lastInsertId().toInt();
    updateProductExpiry(newId, p.expiryDates);
    // initialize stock balance
    updateStockBalance(newId, 0, p.quantity);
    return true;
}

bool Database::updateProduct(const Product& p) {
    QSqlQuery q(m_db);
    q.prepare(R"(UPDATE products SET generic_name=?, brand_name=?, quantity=?,
                 cost_price=?, selling_price=?, barcode=?, updated_at=datetime('now')
                 WHERE id=?)");
    q.addBindValue(p.genericName);
    q.addBindValue(p.brandName);
    q.addBindValue(p.quantity);
    q.addBindValue(p.costPrice);
    q.addBindValue(p.sellingPrice);
    q.addBindValue(p.barcode.isEmpty() ? QVariant() : QVariant(p.barcode));
    q.addBindValue(p.id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    updateProductExpiry(p.id, p.expiryDates);
    return true;
}

bool Database::deleteProduct(int id) {
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM products WHERE id=?");
    q.addBindValue(id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

QList<Product> Database::listProducts(const QString& nameFilter, int limit, int offset) {
    QList<Product> products;
    QSqlQuery q(m_db);
    if (nameFilter.isEmpty()) {
        q.prepare("SELECT * FROM products ORDER BY id LIMIT ? OFFSET ?");
        q.addBindValue(limit);
        q.addBindValue(offset);
    } else {
        q.prepare("SELECT * FROM products WHERE generic_name LIKE ? OR brand_name LIKE ? ORDER BY id LIMIT ? OFFSET ?");
        QString f = "%" + nameFilter + "%";
        q.addBindValue(f);
        q.addBindValue(f);
        q.addBindValue(limit);
        q.addBindValue(offset);
    }
    q.exec();
    while (q.next()) {
        products.append(productFromQuery(q));
    }
    return products;
}

QList<Product> Database::searchProducts(const QString& name, int limit) { return listProducts(name, limit, 0); }

Product Database::getProductById(int id) {
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM products WHERE id=?");
    q.addBindValue(id);
    q.exec();
    if (q.next()) {
        return productFromQuery(q);
    }
    return Product{};
}

Product Database::getProductByBarcode(const QString& barcode) {
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM products WHERE barcode=?");
    q.addBindValue(barcode);
    q.exec();
    if (q.next()) {
        return productFromQuery(q);
    }
    return Product{};
}

int Database::countProducts() {
    QSqlQuery q("SELECT COUNT(*) FROM products", m_db);
    if (q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

bool Database::importProducts(const QList<Product>& products) {
    beginTransaction();
    for (const auto& p : products) {
        if (!createProduct(p)) {
            rollbackTransaction();
            return false;
        }
    }
    return commitTransaction();
}

bool Database::incrementProductQty(int id, int qty) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE products SET quantity=quantity+?, updated_at=datetime('now') WHERE id=?");
    q.addBindValue(qty);
    q.addBindValue(id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool Database::decrementProductQty(int id, int qty) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE products SET quantity=quantity-?, updated_at=datetime('now') WHERE id=? AND quantity>=?");
    q.addBindValue(qty);
    q.addBindValue(id);
    q.addBindValue(qty);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

// =================== TRANSACTIONS ===================

bool Database::beginTransaction() { return m_db.transaction(); }

bool Database::commitTransaction() { return m_db.commit(); }

bool Database::rollbackTransaction() { return m_db.rollback(); }

void Database::updateStockBalance(int productId, int openingQty, int qtyIn) {
    QString today = QDate::currentDate().toString(Qt::ISODate);
    QSqlQuery q(m_db);
    q.prepare("SELECT id FROM stock_balances WHERE product_id=? AND balance_date=?");
    q.addBindValue(productId);
    q.addBindValue(today);
    q.exec();
    if (q.next()) {
        // Update quantity_in
        QSqlQuery upd(m_db);
        upd.prepare("UPDATE stock_balances SET quantity_in=quantity_in+? WHERE product_id=? AND balance_date=?");
        upd.addBindValue(qtyIn);
        upd.addBindValue(productId);
        upd.addBindValue(today);
        upd.exec();
    } else {
        QSqlQuery ins(m_db);
        ins.prepare(
            "INSERT OR IGNORE INTO stock_balances (product_id, opening_quantity, quantity_in, balance_date) VALUES "
            "(?,?,?,?)");
        ins.addBindValue(productId);
        ins.addBindValue(openingQty);
        ins.addBindValue(qtyIn);
        ins.addBindValue(today);
        ins.exec();
    }
}

Transaction Database::transactionFromQuery(QSqlQuery& q) {
    Transaction t;
    t.id = q.value("id").toInt();
    t.userId = q.value("user_id").toInt();
    t.createdAt = QDateTime::fromString(q.value("created_at").toString(), "yyyy-MM-dd hh:mm:ss");

    QByteArray itemsJson = q.value("items").toByteArray();
    QJsonDocument doc = QJsonDocument::fromJson(itemsJson);
    QJsonArray arr = doc.array();
    for (const auto& v : arr) {
        t.items.append(TransactionItem::fromJson(v.toObject()));
    }
    return t;
}

bool Database::createTransaction(const Transaction& t) {
    // Build JSON
    QJsonArray arr;
    for (const auto& item : t.items) {
        arr.append(item.toJson());
    }
    QByteArray itemsJson = QJsonDocument(arr).toJson(QJsonDocument::Compact);

    beginTransaction();

    // Check stock and decrement
    for (const auto& item : t.items) {
        Product p = getProductById(item.productId);
        if (p.id == 0 || p.quantity < item.quantity) {
            m_lastError = QString("Insufficient stock for: %1").arg(item.genericName);
            rollbackTransaction();
            return false;
        }

        // Update stock balance before decrement (record opening)
        QString today = QDate::currentDate().toString(Qt::ISODate);
        QSqlQuery check(m_db);
        check.prepare("SELECT id FROM stock_balances WHERE product_id=? AND balance_date=?");
        check.addBindValue(item.productId);
        check.addBindValue(today);
        check.exec();
        if (!check.next()) {
            updateStockBalance(item.productId, p.quantity, 0);
        }

        QSqlQuery upd(m_db);
        upd.prepare("UPDATE products SET quantity=quantity-?, updated_at=datetime('now') WHERE id=?");
        upd.addBindValue(item.quantity);
        upd.addBindValue(item.productId);
        if (!upd.exec()) {
            m_lastError = upd.lastError().text();
            rollbackTransaction();
            return false;
        }
    }

    QSqlQuery q(m_db);
    q.prepare("INSERT INTO transactions (items, user_id) VALUES (?, ?)");
    q.addBindValue(QString(itemsJson));
    q.addBindValue(t.userId);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        rollbackTransaction();
        return false;
    }

    return commitTransaction();
}

bool Database::deleteTransaction(int id) {
    Transaction t = getTransactionById(id);
    if (t.id == 0) {
        m_lastError = "Transaction not found";
        return false;
    }

    beginTransaction();

    // Re-increment quantities
    for (const auto& item : t.items) {
        QSqlQuery q(m_db);
        q.prepare("UPDATE products SET quantity=quantity+?, updated_at=datetime('now') WHERE id=?");
        q.addBindValue(item.quantity);
        q.addBindValue(item.productId);
        if (!q.exec()) {
            m_lastError = q.lastError().text();
            rollbackTransaction();
            return false;
        }
    }

    QSqlQuery del(m_db);
    del.prepare("DELETE FROM transactions WHERE id=?");
    del.addBindValue(id);
    if (!del.exec()) {
        m_lastError = del.lastError().text();
        rollbackTransaction();
        return false;
    }

    return commitTransaction();
}

QList<Transaction> Database::listTransactions(int limit, int offset) {
    QList<Transaction> list;
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM transactions ORDER BY created_at DESC LIMIT ? OFFSET ?");
    q.addBindValue(limit);
    q.addBindValue(offset);
    q.exec();
    while (q.next()) {
        list.append(transactionFromQuery(q));
    }
    return list;
}

Transaction Database::getTransactionById(int id) {
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM transactions WHERE id=?");
    q.addBindValue(id);
    q.exec();
    if (q.next()) {
        return transactionFromQuery(q);
    }
    return Transaction{};
}

// =================== INVOICES ===================

Invoice Database::invoiceFromQuery(QSqlQuery& q) {
    Invoice inv;
    inv.id = q.value("id").toInt();
    inv.invoiceNumber = q.value("invoice_number").toString();
    inv.purchaseDate = QDate::fromString(q.value("purchase_date").toString(), Qt::ISODate);
    inv.invoiceTotal = q.value("invoice_total").toDouble();
    inv.amountPaid = q.value("amount_paid").toDouble();
    inv.balance = inv.invoiceTotal - inv.amountPaid;
    inv.supplier = q.value("supplier").toString();
    inv.userId = q.value("user_id").toInt();
    inv.createdAt = QDateTime::fromString(q.value("created_at").toString(), "yyyy-MM-dd hh:mm:ss");
    return inv;
}

bool Database::createInvoice(Invoice& inv) {
    QSqlQuery q(m_db);
    q.prepare(R"(INSERT INTO invoices (invoice_number, purchase_date, invoice_total, amount_paid, supplier, user_id)
                 VALUES (?,?,?,?,?,?))");
    q.addBindValue(inv.invoiceNumber);
    q.addBindValue(inv.purchaseDate.toString(Qt::ISODate));
    q.addBindValue(inv.invoiceTotal);
    q.addBindValue(inv.amountPaid);
    q.addBindValue(inv.supplier);
    q.addBindValue(inv.userId);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    inv.id = q.lastInsertId().toInt();
    return true;
}

bool Database::updateInvoice(const Invoice& inv) {
    QSqlQuery q(m_db);
    q.prepare(R"(UPDATE invoices SET invoice_number=?, purchase_date=?, invoice_total=?,
                 amount_paid=?, supplier=?, user_id=? WHERE id=?)");
    q.addBindValue(inv.invoiceNumber);
    q.addBindValue(inv.purchaseDate.toString(Qt::ISODate));
    q.addBindValue(inv.invoiceTotal);
    q.addBindValue(inv.amountPaid);
    q.addBindValue(inv.supplier);
    q.addBindValue(inv.userId);
    q.addBindValue(inv.id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool Database::deleteInvoice(int id) {
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM invoices WHERE id=?");
    q.addBindValue(id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

QList<Invoice> Database::listInvoices(int limit, int offset) {
    QList<Invoice> list;
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM invoices ORDER BY id DESC LIMIT ? OFFSET ?");
    q.addBindValue(limit);
    q.addBindValue(offset);
    q.exec();
    while (q.next()) {
        list.append(invoiceFromQuery(q));
    }
    return list;
}

Invoice Database::getInvoiceById(int id) {
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM invoices WHERE id=?");
    q.addBindValue(id);
    q.exec();
    if (q.next()) {
        return invoiceFromQuery(q);
    }
    return Invoice{};
}

Invoice Database::getInvoiceByNumber(const QString& num) {
    QSqlQuery q(m_db);
    q.prepare("SELECT * FROM invoices WHERE invoice_number=? LIMIT 1");
    q.addBindValue(num);
    q.exec();
    if (q.next()) {
        return invoiceFromQuery(q);
    }
    return Invoice{};
}

// =================== STOCK IN ===================

StockInItem Database::stockInFromQuery(QSqlQuery& q) {
    StockInItem s;
    s.id = q.value("id").toInt();
    s.productId = q.value("product_id").toInt();
    s.invoiceId = q.value("invoice_id").toInt();
    s.quantity = q.value("quantity").toInt();
    s.costPrice = q.value("cost_price").toDouble();
    s.expiryDate = QDate::fromString(q.value("expiry_date").toString(), Qt::ISODate);
    s.comment = q.value("comment").toString();
    s.createdAt = QDateTime::fromString(q.value("created_at").toString(), "yyyy-MM-dd hh:mm:ss");
    // joined
    QVariant gn = q.value("generic_name");
    QVariant bn = q.value("brand_name");
    if (gn.isValid()) {
        s.genericName = gn.toString();
    }
    if (bn.isValid()) {
        s.brandName = bn.toString();
    }
    return s;
}

bool Database::addStockIn(const StockInItem& item) {
    beginTransaction();

    QSqlQuery q(m_db);
    q.prepare(R"(INSERT INTO stock_in (product_id, invoice_id, quantity, cost_price, expiry_date, comment)
                 VALUES (?,?,?,?,?,?))");
    q.addBindValue(item.productId);
    q.addBindValue(item.invoiceId);
    q.addBindValue(item.quantity);
    q.addBindValue(item.costPrice);
    q.addBindValue(item.expiryDate.isValid() ? item.expiryDate.toString(Qt::ISODate) : QString(""));
    q.addBindValue(item.comment);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        rollbackTransaction();
        return false;
    }

    // Update product quantity
    Product p = getProductById(item.productId);
    QSqlQuery upd(m_db);
    upd.prepare("UPDATE products SET quantity=quantity+?, updated_at=datetime('now') WHERE id=?");
    upd.addBindValue(item.quantity);
    upd.addBindValue(item.productId);
    if (!upd.exec()) {
        m_lastError = upd.lastError().text();
        rollbackTransaction();
        return false;
    }

    // Update expiry dates
    if (item.expiryDate.isValid()) {
        if (p.quantity <= 0) {
            // Replace expiry dates
            updateProductExpiry(item.productId, {item.expiryDate});
        } else {
            addProductExpiry(item.productId, item.expiryDate);
        }
    }

    // Update stock balance
    updateStockBalance(item.productId, p.quantity, item.quantity);

    return commitTransaction();
}

bool Database::deleteStockIn(int id) {
    StockInItem si = getStockInById(id);
    if (si.id == 0) {
        m_lastError = "StockIn not found";
        return false;
    }

    beginTransaction();

    // Decrement product quantity
    QSqlQuery upd(m_db);
    upd.prepare("UPDATE products SET quantity=MAX(0, quantity-?), updated_at=datetime('now') WHERE id=?");
    upd.addBindValue(si.quantity);
    upd.addBindValue(si.productId);
    if (!upd.exec()) {
        m_lastError = upd.lastError().text();
        rollbackTransaction();
        return false;
    }

    // Remove expiry date
    if (si.expiryDate.isValid()) {
        removeProductExpiry(si.productId, si.expiryDate);
    }

    QSqlQuery del(m_db);
    del.prepare("DELETE FROM stock_in WHERE id=?");
    del.addBindValue(id);
    if (!del.exec()) {
        m_lastError = del.lastError().text();
        rollbackTransaction();
        return false;
    }

    return commitTransaction();
}

QList<StockInItem> Database::getStockInByInvoice(int invoiceId) {
    QList<StockInItem> list;
    QSqlQuery q(m_db);
    q.prepare(R"(SELECT si.*, p.generic_name, p.brand_name
                 FROM stock_in si JOIN products p ON si.product_id=p.id
                 WHERE si.invoice_id=? ORDER BY si.id)");
    q.addBindValue(invoiceId);
    q.exec();
    while (q.next()) {
        list.append(stockInFromQuery(q));
    }
    return list;
}

StockInItem Database::getStockInById(int id) {
    QSqlQuery q(m_db);
    q.prepare(R"(SELECT si.*, p.generic_name, p.brand_name
                 FROM stock_in si JOIN products p ON si.product_id=p.id
                 WHERE si.id=?)");
    q.addBindValue(id);
    q.exec();
    if (q.next()) {
        return stockInFromQuery(q);
    }
    return StockInItem{};
}

// =================== REPORTS ===================

QList<SalesReport> Database::getDailySalesReports(const QString& dateFilter) {
    QList<SalesReport> list;
    QSqlQuery q(m_db);
    QString sql = R"(
        SELECT
            date(t.created_at) AS transaction_date,
            SUM(
                CAST(json_extract(item.value, '$.quantity') AS REAL) *
                CAST(json_extract(item.value, '$.selling_price') AS REAL)
            ) AS total_income
        FROM transactions t, json_each(t.items) AS item
    )";

    if (!dateFilter.isEmpty()) {
        sql += " WHERE date(t.created_at) = '" + dateFilter + "'";
    }
    sql += " GROUP BY date(t.created_at) ORDER BY transaction_date DESC";

    if (!q.exec(sql)) {
        m_lastError = q.lastError().text();
        qWarning() << "getDailySalesReports error:" << m_lastError;
        return list;
    }
    while (q.next()) {
        SalesReport r;
        r.transactionDate = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        r.totalIncome = q.value(1).toDouble();
        list.append(r);
    }
    return list;
}

QList<MonthlySalesReport> Database::getMonthlySalesReports() {
    QList<MonthlySalesReport> list;
    QString sql = R"(
        SELECT
            strftime('%Y-%m-01', t.created_at) AS month,
            SUM(
                CAST(json_extract(item.value, '$.quantity') AS REAL) *
                CAST(json_extract(item.value, '$.selling_price') AS REAL)
            ) AS total_income
        FROM transactions t, json_each(t.items) AS item
        GROUP BY strftime('%Y-%m', t.created_at)
        ORDER BY month DESC
    )";
    QSqlQuery q(m_db);
    if (!q.exec(sql)) {
        m_lastError = q.lastError().text();
        qWarning() << "getMonthlySalesReports error:" << m_lastError;
        return list;
    }
    while (q.next()) {
        MonthlySalesReport r;
        r.month = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        r.totalIncome = q.value(1).toDouble();
        list.append(r);
    }
    return list;
}

QList<AnnualSalesReport> Database::getAnnualSalesReports() {
    QList<AnnualSalesReport> list;
    QString sql = R"(
        SELECT
            strftime('%Y-01-01', t.created_at) AS yr,
            SUM(
                CAST(json_extract(item.value, '$.quantity') AS REAL) *
                CAST(json_extract(item.value, '$.selling_price') AS REAL)
            ) AS total_income
        FROM transactions t, json_each(t.items) AS item
        GROUP BY strftime('%Y', t.created_at)
        ORDER BY yr DESC
    )";
    QSqlQuery q(m_db);
    if (!q.exec(sql)) {
        m_lastError = q.lastError().text();
        qWarning() << "getAnnualSalesReports error:" << m_lastError;
        return list;
    }
    while (q.next()) {
        AnnualSalesReport r;
        r.year = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        r.totalIncome = q.value(1).toDouble();
        list.append(r);
    }
    return list;
}

QList<ProductSale> Database::getDailyProductSales(const QDate& date) {
    QList<ProductSale> list;
    QString sql = R"(
        SELECT
            date(t.created_at) AS transaction_date,
            CAST(json_extract(item.value, '$.id') AS INTEGER) AS product_id,
            json_extract(item.value, '$.generic_name') AS product_name,
            CAST(json_extract(item.value, '$.cost_price') AS REAL) AS cost_price,
            CAST(json_extract(item.value, '$.selling_price') AS REAL) AS selling_price,
            SUM(CAST(json_extract(item.value, '$.quantity') AS INTEGER)) AS quantity_sold,
            SUM(CAST(json_extract(item.value,'$.quantity') AS REAL)*CAST(json_extract(item.value,'$.selling_price') AS REAL)) AS income,
            SUM(CAST(json_extract(item.value,'$.quantity') AS REAL)*(CAST(json_extract(item.value,'$.selling_price') AS REAL)-CAST(json_extract(item.value,'$.cost_price') AS REAL))) AS profit
        FROM transactions t, json_each(t.items) AS item
        WHERE date(t.created_at) = ?
        GROUP BY product_id, product_name
        ORDER BY income DESC
    )";
    QSqlQuery q(m_db);
    q.prepare(sql);
    q.addBindValue(date.toString(Qt::ISODate));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        qWarning() << "getDailyProductSales error:" << m_lastError;
        return list;
    }
    while (q.next()) {
        ProductSale p;
        p.transactionDate = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        p.productId = q.value(1).toInt();
        p.productName = q.value(2).toString();
        p.costPrice = q.value(3).toDouble();
        p.sellingPrice = q.value(4).toDouble();
        p.quantitySold = q.value(5).toInt();
        p.income = q.value(6).toDouble();
        p.profit = q.value(7).toDouble();
        list.append(p);
    }
    return list;
}

QList<ProductSale> Database::getMonthlyProductSales(int year, int month) {
    QList<ProductSale> list;
    QString sql = R"(
        SELECT
            strftime('%Y-%m-01', t.created_at) AS transaction_date,
            CAST(json_extract(item.value, '$.id') AS INTEGER) AS product_id,
            json_extract(item.value, '$.generic_name') AS product_name,
            CAST(json_extract(item.value, '$.cost_price') AS REAL) AS cost_price,
            CAST(json_extract(item.value, '$.selling_price') AS REAL) AS selling_price,
            SUM(CAST(json_extract(item.value, '$.quantity') AS INTEGER)) AS quantity_sold,
            SUM(CAST(json_extract(item.value,'$.quantity') AS REAL)*CAST(json_extract(item.value,'$.selling_price') AS REAL)) AS income,
            SUM(CAST(json_extract(item.value,'$.quantity') AS REAL)*(CAST(json_extract(item.value,'$.selling_price') AS REAL)-CAST(json_extract(item.value,'$.cost_price') AS REAL))) AS profit
        FROM transactions t, json_each(t.items) AS item
        WHERE strftime('%Y', t.created_at) = ? AND strftime('%m', t.created_at) = ?
        GROUP BY product_id, product_name
        ORDER BY income DESC
    )";
    QSqlQuery q(m_db);
    q.prepare(sql);
    q.addBindValue(QString::number(year));
    q.addBindValue(QString("%1").arg(month, 2, 10, QChar('0')));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return list;
    }
    while (q.next()) {
        ProductSale p;
        p.transactionDate = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        p.productId = q.value(1).toInt();
        p.productName = q.value(2).toString();
        p.costPrice = q.value(3).toDouble();
        p.sellingPrice = q.value(4).toDouble();
        p.quantitySold = q.value(5).toInt();
        p.income = q.value(6).toDouble();
        p.profit = q.value(7).toDouble();
        list.append(p);
    }
    return list;
}

QList<ProductSale> Database::getAnnualProductSales(int year) {
    QList<ProductSale> list;
    QString sql = R"(
        SELECT
            strftime('%Y-01-01', t.created_at) AS transaction_date,
            CAST(json_extract(item.value, '$.id') AS INTEGER) AS product_id,
            json_extract(item.value, '$.generic_name') AS product_name,
            CAST(json_extract(item.value, '$.cost_price') AS REAL) AS cost_price,
            CAST(json_extract(item.value, '$.selling_price') AS REAL) AS selling_price,
            SUM(CAST(json_extract(item.value, '$.quantity') AS INTEGER)) AS quantity_sold,
            SUM(CAST(json_extract(item.value,'$.quantity') AS REAL)*CAST(json_extract(item.value,'$.selling_price') AS REAL)) AS income,
            SUM(CAST(json_extract(item.value,'$.quantity') AS REAL)*(CAST(json_extract(item.value,'$.selling_price') AS REAL)-CAST(json_extract(item.value,'$.cost_price') AS REAL))) AS profit
        FROM transactions t, json_each(t.items) AS item
        WHERE strftime('%Y', t.created_at) = ?
        GROUP BY product_id, product_name
        ORDER BY income DESC
    )";
    QSqlQuery q(m_db);
    q.prepare(sql);
    q.addBindValue(QString::number(year));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return list;
    }
    while (q.next()) {
        ProductSale p;
        p.transactionDate = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        p.productId = q.value(1).toInt();
        p.productName = q.value(2).toString();
        p.costPrice = q.value(3).toDouble();
        p.sellingPrice = q.value(4).toDouble();
        p.quantitySold = q.value(5).toInt();
        p.income = q.value(6).toDouble();
        p.profit = q.value(7).toDouble();
        list.append(p);
    }
    return list;
}

QList<StockCard> Database::getStockCard(const QDate& fromDate, const QDate& toDate) {
    QList<StockCard> list;
    QString sql = R"(
        SELECT
            sb.balance_date,
            p.id AS product_id,
            p.generic_name,
            p.brand_name,
            sb.opening_quantity,
            sb.quantity_in,
            COALESCE((
                SELECT SUM(CAST(json_extract(item.value, '$.quantity') AS INTEGER))
                FROM transactions t, json_each(t.items) AS item
                WHERE date(t.created_at) = sb.balance_date
                  AND CAST(json_extract(item.value, '$.id') AS INTEGER) = p.id
            ), 0) AS quantity_out
        FROM stock_balances sb
        JOIN products p ON sb.product_id = p.id
        WHERE sb.balance_date BETWEEN ? AND ?
        ORDER BY sb.balance_date DESC, p.generic_name
    )";
    QSqlQuery q(m_db);
    q.prepare(sql);
    q.addBindValue(fromDate.toString(Qt::ISODate));
    q.addBindValue(toDate.toString(Qt::ISODate));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return list;
    }
    while (q.next()) {
        StockCard sc;
        sc.date = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        sc.productId = q.value(1).toInt();
        sc.genericName = q.value(2).toString();
        sc.brandName = q.value(3).toString();
        sc.openingQuantity = q.value(4).toInt();
        sc.quantityIn = q.value(5).toInt();
        sc.quantityOut = q.value(6).toInt();
        sc.closingQuantity = sc.openingQuantity + sc.quantityIn - sc.quantityOut;
        list.append(sc);
    }
    return list;
}

QList<Product> Database::getMostCommonProducts(int limit) {
    QList<Product> list;
    QString sql = R"(
        SELECT
            CAST(json_extract(item.value, '$.id') AS INTEGER) AS product_id,
            COUNT(*) AS cnt
        FROM transactions t, json_each(t.items) AS item
        GROUP BY product_id
        ORDER BY cnt DESC
        LIMIT ?
    )";
    QSqlQuery q(m_db);
    q.prepare(sql);
    q.addBindValue(limit);
    if (!q.exec()) {
        // Fallback: just return first N products
        return listProducts(QString(), limit, 0);
    }
    while (q.next()) {
        int pid = q.value(0).toInt();
        Product p = getProductById(pid);
        if (p.id > 0) {
            list.append(p);
        }
    }
    // if empty, return all
    if (list.isEmpty()) {
        return listProducts(QString(), limit, 0);
    }
    return list;
}
