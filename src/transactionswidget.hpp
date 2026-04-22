#pragma once

#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QWidget>
#include "models.hpp"

class TransactionDetailWidget : public QWidget {
    Q_OBJECT
  public:
    explicit TransactionDetailWidget(const Transaction& t, const User& user, QWidget* parent = nullptr);
  signals:
    void backRequested();
    void transactionDeleted();

  private:
    Transaction m_transaction;
    User m_user;
    void setupUi();
};

class TransactionsWidget : public QWidget {
    Q_OBJECT
  public:
    explicit TransactionsWidget(const User& user, QWidget* parent = nullptr);
    void refresh();
  private slots:
    void onViewDetails(int id);

  private:
    User m_currentUser;
    QTableWidget* m_table;
    QStackedWidget* m_stack;
    TransactionDetailWidget* m_detailWidget = nullptr;
    void setupUi();
    void loadData();
    void setRow(int row, const Transaction& t);
};
