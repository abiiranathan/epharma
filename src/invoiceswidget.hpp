#pragma once

#include <QDateEdit>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QWidget>
#include "models.hpp"

class InvoiceDialog : public QDialog {
    Q_OBJECT
  public:
    explicit InvoiceDialog(QWidget* parent = nullptr, const Invoice& inv = Invoice{});
    [[nodiscard]] Invoice getInvoice() const;

  private:
    QLineEdit* m_invoiceNumber;
    QDateEdit* m_purchaseDate;
    QDoubleSpinBox* m_invoiceTotal;
    QDoubleSpinBox* m_amountPaid;
    QLineEdit* m_supplier;
    bool m_isEdit;
    int m_invoiceId;
};

class StockInDialog : public QDialog {
    Q_OBJECT
  public:
    explicit StockInDialog(int invoiceId, QWidget* parent = nullptr);
    [[nodiscard]] StockInItem getStockIn() const;

  private:
    QLineEdit* m_productSearch;
    QLabel* m_productLabel;
    int m_selectedProductId = 0;
    QSpinBox* m_quantity;
    QDoubleSpinBox* m_costPrice;
    QDateEdit* m_expiryDate;
    QLineEdit* m_comment;
    int m_invoiceId;
};

class InvoiceDetailWidget : public QWidget {
    Q_OBJECT
  public:
    explicit InvoiceDetailWidget(const Invoice& inv, const User& user, QWidget* parent = nullptr);
  signals:
    void backRequested();
  private slots:
    void onAddStockIn();
    void onDeleteStockIn(int id);
    void refreshItems();

  private:
    Invoice m_invoice;
    User m_user;
    QTableWidget* m_itemsTable;
    QLabel* m_summaryLabel;
    void setupUi();
};

class InvoicesWidget : public QWidget {
    Q_OBJECT
  public:
    explicit InvoicesWidget(const User& user, QWidget* parent = nullptr);
    void refresh();
  private slots:
    void onAdd();
    void onEdit();
    void onDelete();
    void onViewDetails(int id);
    void onSearch(const QString& text);

  private:
    User m_currentUser;
    QTableWidget* m_table;
    QLineEdit* m_searchEdit;
    QStackedWidget* m_stack;
    InvoiceDetailWidget* m_detailWidget = nullptr;
    void setupUi();
    void loadData();
    void setRow(int row, const Invoice& inv);
};
