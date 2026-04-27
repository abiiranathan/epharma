#include "loginwindow.hpp"
#include <QApplication>
#include <QCryptographicHash>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QScreen>
#include <QShortcut>
#include <QVBoxLayout>
#include "database.hpp"

LoginWindow::LoginWindow(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Tella POS — Login");
    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    setFixedSize(420, 480);
    setupUi();

    // Center on screen
    if (QScreen* s = QApplication::primaryScreen()) {
        QRect sg = s->availableGeometry();
        move(sg.center() - rect().center());
    }
}

void LoginWindow::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Top banner
    auto* banner = new QWidget;
    banner->setFixedHeight(80);
    banner->setStyleSheet("background-color: #1e3a5f;");
    auto* bannerLayout = new QVBoxLayout(banner);
    bannerLayout->setContentsMargins(24, 16, 24, 16);

    auto* titleLbl = new QLabel("Tella POS");
    titleLbl->setStyleSheet("color: #ffffff; font-size: 22px; font-weight: 700;");
    auto* subLbl = new QLabel("Point of Sale System");
    subLbl->setStyleSheet("color: #8ab4d8; font-size: 12px;");
    bannerLayout->addWidget(titleLbl);
    bannerLayout->addWidget(subLbl);
    root->addWidget(banner);

    // Card
    auto* card = new QWidget;
    card->setStyleSheet("background-color: #ffffff;");
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(32, 32, 32, 32);
    cardLayout->setSpacing(16);

    auto* cardTitle = new QLabel("Sign in to your account");
    cardTitle->setStyleSheet("font-size: 16px; font-weight: 600; color: #1e3a5f;");
    cardLayout->addWidget(cardTitle);

    // Error label
    m_errorLabel = new QLabel;
    m_errorLabel->setStyleSheet(
        "color: #c53030; background-color: #fff5f5; border: 1px solid #fed7d7;"
        "border-radius: 6px; padding: 8px 12px; font-size: 13px;");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();
    cardLayout->addWidget(m_errorLabel);

    // Username
    auto* userLabel = new QLabel("Username");
    userLabel->setStyleSheet("font-weight: 600; color: #4a5568; font-size: 13px; margin-bottom: 2px;");
    m_usernameEdit = new QLineEdit;
    m_usernameEdit->setPlaceholderText("Enter your username");
    m_usernameEdit->setObjectName("searchBar");

    // Password
    auto* passLabel = new QLabel("Password");
    passLabel->setStyleSheet("font-weight: 600; color: #4a5568; font-size: 13px; margin-bottom: 2px;");
    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("Enter your password");
    m_passwordEdit->setObjectName("searchBar");

    cardLayout->addWidget(userLabel);
    cardLayout->addWidget(m_usernameEdit);
    cardLayout->addWidget(passLabel);
    cardLayout->addWidget(m_passwordEdit);
    cardLayout->addSpacing(8);

    m_loginBtn = new QPushButton("Sign In");
    m_loginBtn->setFixedHeight(40);
    m_loginBtn->setDefault(true);
    cardLayout->addWidget(m_loginBtn);

    auto* hint = new QLabel("Default: admin / admin123");
    hint->setStyleSheet("color: #38557c; font-size: 11px;");
    hint->setVisible(false);

    // enable if user presses Ctrl+H on the login screen
    hint->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(hint);
    cardLayout->addStretch();

    // Ctrl+H to toggle hint visibility
    auto* hintShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_H), this);
    connect(hintShortcut, &QShortcut::activated, this, [hint] { hint->setVisible(!hint->isVisible()); });

    root->addWidget(card);

    connect(m_loginBtn, &QPushButton::clicked, this, &LoginWindow::onLogin);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginWindow::onLogin);
    connect(m_usernameEdit, &QLineEdit::returnPressed, this, [this] { m_passwordEdit->setFocus(); });
    m_usernameEdit->setFocus();
}

void LoginWindow::onLogin() {
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        m_errorLabel->setText("Please enter username and password.");
        m_errorLabel->show();
        return;
    }

    User user = Database::instance().getUserByUsername(username);
    if (user.id == 0) {
        m_errorLabel->setText("Invalid username or password.");
        m_errorLabel->show();
        m_passwordEdit->clear();
        return;
    }

    QString hashedPw = QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
    if (user.password != hashedPw) {
        m_errorLabel->setText("Invalid username or password.");
        m_errorLabel->show();
        m_passwordEdit->clear();
        return;
    }

    if (!user.isActive) {
        m_errorLabel->setText("Your account has been deactivated. Contact administrator.");
        m_errorLabel->show();
        return;
    }

    m_user = user;
    accept();
}
