#pragma once

#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>
#include "models.hpp"

class POSWidget : public QWidget {
    Q_OBJECT
  public:
    explicit POSWidget(User user, QWidget* parent = nullptr);

  private slots:
    void onBarcodeEntered();
    void onSearchChanged(const QString& text);
    void addProductToQueue(const Product& product);
    void removeFromQueue(int row);
    void saveTransaction();
    void clearQueue();
    void onProductDoubleClicked(int row, int col);
    void updateQueueSubtotal(int row, int col);

  private:
    User m_currentUser;

    // Left panel - product search
    QTableWidget* m_productsTable;
    QLineEdit* m_searchEdit;
    QLineEdit* m_barcodeEdit;

    // Right panel - receipt/queue
    QTableWidget* m_queueTable;
    QLabel* m_totalLabel;
    QPushButton* m_saveBtn;
    QPushButton* m_clearBtn;

    QList<Product> m_currentProducts;
    QList<TransactionItem> m_queueItems;

    QTimer* m_barcodeTimer;
    QString m_barcodeBuffer;

    bool eventFilter(QObject* obj, QEvent* event) override;
    void setupUi();
    void loadProducts(const QString& filter = QString());
    void updateTotal();
    [[nodiscard]] double computeTotal() const;

    static QString formatCurrency(double val) { return QString("UGX %L1").arg(val, 0, 'f', 2); }
};
