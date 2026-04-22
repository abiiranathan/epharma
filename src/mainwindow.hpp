#pragma once

#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QVBoxLayout>
#include "models.hpp"

// Forward declarations for page widgets
class POSWidget;
class ProductsWidget;
class InvoicesWidget;
class TransactionsWidget;
class ReportsWidget;
class UsersWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    explicit MainWindow(const User& user, QWidget* parent = nullptr);

  private slots:
    void switchPage(int index);
    void onLogout();

  private:
    User m_currentUser;
    QStackedWidget* m_stack;
    QList<QPushButton*> m_navBtns;

    POSWidget* m_posWidget;
    ProductsWidget* m_productsWidget;
    InvoicesWidget* m_invoicesWidget;
    TransactionsWidget* m_transactionsWidget;
    ReportsWidget* m_reportsWidget;
    UsersWidget* m_usersWidget;

    QLabel* m_userLabel;

    void setupUi();
    void addNavButton(QWidget* sidebar, QVBoxLayout* layout, const QString& icon, const QString& text, int index);
};
