#pragma once

#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>
#include "models.hpp"

class UserDialog : public QDialog {
    Q_OBJECT
  public:
    explicit UserDialog(QWidget* parent = nullptr, const User& user = User{});
    QString getUsername() const;
    QString getPassword() const;
    bool isAdmin() const;
    bool shouldUpdatePassword() const;

  private:
    QLineEdit* m_username;
    QLineEdit* m_password;
    QLineEdit* m_confirm;
    QCheckBox* m_adminCheck;
    bool m_isEdit;
};

class UsersWidget : public QWidget {
    Q_OBJECT
  public:
    explicit UsersWidget(const User& currentUser, QWidget* parent = nullptr);
    void refresh();
  private slots:
    void onAdd();

  private:
    User m_currentUser;
    QTableWidget* m_table;
    void setupUi();
    void loadData();
    void setRow(int row, const User& u);
};
