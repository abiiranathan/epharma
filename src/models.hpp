#pragma once

#include <QDate>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>

struct User {
    int id = 0;
    QString username;
    QString password;
    bool isActive = true;
    bool isAdmin = false;
    QDateTime createdAt;
};

struct Product {
    int id = 0;
    QString genericName;
    QString brandName;
    int quantity = 0;
    double costPrice = 0.0;
    double sellingPrice = 0.0;
    QList<QDate> expiryDates;
    QString barcode;
    QDateTime createdAt;
    QDateTime updatedAt;

    [[nodiscard]] QJsonObject toJson() const;
    static Product fromJson(const QJsonObject& obj);

    [[nodiscard]] QString displayName() const {
        if (brandName.isEmpty()) {
            return genericName;
        }
        return genericName + " (" + brandName + ")";
    }
};

struct TransactionItem {
    int productId = 0;
    QString genericName;
    QString brandName;
    int quantity = 0;
    double sellingPrice = 0.0;
    double costPrice = 0.0;
    QString barcode;

    [[nodiscard]] double subtotal() const { return quantity * sellingPrice; }
    [[nodiscard]] QJsonObject toJson() const;
    static TransactionItem fromJson(const QJsonObject& obj);
};

struct Transaction {
    int id = 0;
    QList<TransactionItem> items;
    QDateTime createdAt;
    int userId = 0;

    [[nodiscard]] double total() const {
        double t = 0;
        for (const auto& i : items) {
            t += i.subtotal();
        }
        return t;
    }
};

struct Invoice {
    int id = 0;
    QString invoiceNumber;
    QDate purchaseDate;
    double invoiceTotal = 0.0;
    double amountPaid = 0.0;
    double balance = 0.0;
    QString supplier;
    int userId = 0;
    QDateTime createdAt;
};

struct StockInItem {
    int id = 0;
    int productId = 0;
    int invoiceId = 0;
    int quantity = 0;
    double costPrice = 0.0;
    QDate expiryDate;
    QString comment;
    QDateTime createdAt;
    // joined fields
    QString genericName;
    QString brandName;
};

struct StockBalance {
    int id = 0;
    int productId = 0;
    int openingQuantity = 0;
    int quantityIn = 0;
    int quantityOut = 0;
    int quantityReversal = 0;
    QDateTime createdAt;
};

struct StockCard {
    QDate date;
    int productId = 0;
    QString genericName;
    QString brandName;
    int openingQuantity = 0;
    int quantityIn = 0;
    int quantityOut = 0;
    int quantityReversal = 0;
    int closingQuantity = 0;
};

struct ProductSale {
    QDate transactionDate;
    int productId = 0;
    QString productName;
    double costPrice = 0.0;
    double sellingPrice = 0.0;
    int quantitySold = 0;
    double income = 0.0;
    double profit = 0.0;
};

struct SalesReport {
    QDate transactionDate;
    double totalIncome = 0.0;
};

struct MonthlySalesReport {
    QDate month;
    double totalIncome = 0.0;
};

struct AnnualSalesReport {
    QDate year;
    double totalIncome = 0.0;
};
