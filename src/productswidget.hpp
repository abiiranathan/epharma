#pragma once

#include <QDateEdit>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QWidget>
#include "models.hpp"

class ProductDialog : public QDialog {
    Q_OBJECT
  public:
    explicit ProductDialog(QWidget* parent = nullptr, const Product& product = Product{});
    [[nodiscard]] Product getProduct() const;

  private:
    QLineEdit* m_genericName;
    QLineEdit* m_brandName;
    QSpinBox* m_quantity;
    QDoubleSpinBox* m_costPrice;
    QDoubleSpinBox* m_sellingPrice;
    QLineEdit* m_barcode;
    QLineEdit* m_expiryDates;
    bool m_isEdit;
    int m_productId;
};

class ProductsWidget : public QWidget {
    Q_OBJECT
  public:
    explicit ProductsWidget(const User& user, QWidget* parent = nullptr);
    void refresh();

  private slots:
    void onSearch(const QString& text);
    void onAdd();
    void onEdit();
    void onDelete();
    void onImport();
    void onPrevPage();
    void onNextPage();

  private:
    User m_currentUser;
    QTableWidget* m_table;
    QLineEdit* m_searchEdit;
    QLabel* m_pageLabel;
    QPushButton* m_prevBtn;
    QPushButton* m_nextBtn;
    QLabel* m_countLabel;

    int m_page = 1;
    int m_pageSize = 50;
    int m_totalCount = 0;
    QString m_searchFilter;

    void setupUi();
    void loadPage();
    void setRow(int row, const Product& p);
    [[nodiscard]] int selectedId() const;
};
