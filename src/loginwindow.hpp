#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include "models.hpp"

class LoginWindow : public QDialog {
    Q_OBJECT
  public:
    explicit LoginWindow(QWidget* parent = nullptr);
    [[nodiscard]] User loggedInUser() const { return m_user; }

  private slots:
    void onLogin();

  private:
    QLineEdit* m_usernameEdit;
    QLineEdit* m_passwordEdit;
    QLabel* m_errorLabel;
    QPushButton* m_loginBtn;
    User m_user;

    void setupUi();
};
