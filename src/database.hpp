#pragma once

#include <QDate>
#include <QList>
#include <QString>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include "models.hpp"

class Database {
  public:
    static Database& instance();

    bool open(const QString& path);
    void close();
    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] QString lastError() const;

    bool initSchema();

    // Users
    bool createUser(const QString& username, const QString& password, bool isAdmin = false);
    bool updateUser(int id, const QString& username, const QString& newPassword, bool updatePassword);
    bool deleteUser(int id);
    bool activateUser(int id);
    bool deactivateUser(int id);
    bool promoteUser(int id);
    bool demoteUser(int id);
    QList<User> listUsers();
    User getUserById(int id);
    User getUserByUsername(const QString& username);

    // Products
    bool createProduct(const Product& p);
    bool updateProduct(const Product& p);
    bool deleteProduct(int id);
    QList<Product> listProducts(const QString& nameFilter = QString(), int limit = 50, int offset = 0);
    QList<Product> searchProducts(const QString& name, int limit = 50);
    Product getProductById(int id);
    Product getProductByBarcode(const QString& barcode);
    int countProducts();
    bool importProducts(const QList<Product>& products);
    bool incrementProductQty(int id, int qty);
    bool decrementProductQty(int id, int qty);

    // Transactions
    bool createTransaction(const Transaction& t);
    bool deleteTransaction(int id);
    QList<Transaction> listTransactions(int limit = 50, int offset = 0);
    Transaction getTransactionById(int id);

    // Invoices
    bool createInvoice(Invoice& inv);
    bool updateInvoice(const Invoice& inv);
    bool deleteInvoice(int id);
    QList<Invoice> listInvoices(int limit = 50, int offset = 0);
    Invoice getInvoiceById(int id);
    Invoice getInvoiceByNumber(const QString& num);

    // Stock In
    bool addStockIn(const StockInItem& item);
    bool deleteStockIn(int id);
    QList<StockInItem> getStockInByInvoice(int invoiceId);
    StockInItem getStockInById(int id);

    // Reports
    QList<SalesReport> getDailySalesReports(const QString& dateFilter = QString());
    QList<MonthlySalesReport> getMonthlySalesReports();
    QList<AnnualSalesReport> getAnnualSalesReports();
    QList<ProductSale> getDailyProductSales(const QDate& date);
    QList<ProductSale> getMonthlyProductSales(int year, int month);
    QList<ProductSale> getAnnualProductSales(int year);
    QList<StockCard> getStockCard(const QDate& fromDate, const QDate& toDate);

    // Most common products
    QList<Product> getMostCommonProducts(int limit = 10);

    // Transaction helpers
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // Deleted constructors/operators MUST be public to prevent misuse by other classes, but we make them private to
    // prevent instantiation
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

  private:
    Database() = default;
    ~Database() = default;

    QSqlDatabase m_db;
    QString m_lastError;

    Product productFromQuery(QSqlQuery& q);
    User userFromQuery(QSqlQuery& q);
    Invoice invoiceFromQuery(QSqlQuery& q);
    StockInItem stockInFromQuery(QSqlQuery& q);
    Transaction transactionFromQuery(QSqlQuery& q);

    bool updateProductExpiry(int productId, const QList<QDate>& dates);
    bool addProductExpiry(int productId, const QDate& date);
    bool removeProductExpiry(int productId, const QDate& date);
    QList<QDate> getProductExpiry(int productId);

    void updateStockBalance(int productId, int openingQty, int qtyIn);
};
