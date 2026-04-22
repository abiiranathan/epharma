#include "userswidget.hpp"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>
#include "database.hpp"

// =================== UserDialog ===================

UserDialog::UserDialog(QWidget* parent, const User& user) : QDialog(parent), m_isEdit(user.id > 0) {
    setWindowTitle(m_isEdit ? "Edit User" : "Create User Account");
    setMinimumWidth(380);

    auto* layout = new QVBoxLayout(this);

    auto* title = new QLabel(m_isEdit ? "✏️  Edit User" : "👤  New User Account");
    title->setStyleSheet("font-size: 16px; font-weight: 700; color: #1e3a5f; margin-bottom: 8px;");
    layout->addWidget(title);

    auto* form = new QFormLayout;
    form->setSpacing(10);

    m_username = new QLineEdit(user.username);
    m_username->setPlaceholderText("Enter unique username");
    form->addRow("Username*:", m_username);

    m_password = new QLineEdit;
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setPlaceholderText(m_isEdit ? "Leave blank to keep current" : "Min 8 characters");
    form->addRow("Password*:", m_password);

    m_confirm = new QLineEdit;
    m_confirm->setEchoMode(QLineEdit::Password);
    m_confirm->setPlaceholderText("Confirm password");
    form->addRow("Confirm Password:", m_confirm);

    m_adminCheck = new QCheckBox("Grant Administrator privileges");
    m_adminCheck->setChecked(user.isAdmin);
    form->addRow("Role:", m_adminCheck);

    layout->addLayout(form);
    layout->addSpacing(8);

    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    btnBox->button(QDialogButtonBox::Ok)->setText(m_isEdit ? "Update" : "Create Account");
    btnBox->button(QDialogButtonBox::Ok)->setObjectName("successBtn");
    connect(btnBox, &QDialogButtonBox::accepted, this, [this] {
        if (m_username->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Validation", "Username is required.");
            return;
        }
        if (!m_isEdit && m_password->text().length() < 8) {
            QMessageBox::warning(this, "Validation", "Password must be at least 8 characters.");
            return;
        }
        if (!m_password->text().isEmpty() && m_password->text() != m_confirm->text()) {
            QMessageBox::warning(this, "Validation", "Passwords do not match.");
            return;
        }
        accept();
    });
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

QString UserDialog::getUsername() const { return m_username->text().trimmed(); }
QString UserDialog::getPassword() const { return m_password->text(); }
bool UserDialog::isAdmin() const { return m_adminCheck->isChecked(); }
bool UserDialog::shouldUpdatePassword() const { return !m_password->text().isEmpty(); }

// =================== UsersWidget ===================

UsersWidget::UsersWidget(const User& currentUser, QWidget* parent) : QWidget(parent), m_currentUser(currentUser) {
    setupUi();
}

void UsersWidget::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto* headerRow = new QHBoxLayout;
    auto* title = new QLabel("👥  User Management");
    title->setObjectName("pageTitle");
    headerRow->addWidget(title);
    headerRow->addStretch();

    auto* addBtn = new QPushButton("➕  New User");
    addBtn->setObjectName("successBtn");
    addBtn->setFixedHeight(34);
    headerRow->addWidget(addBtn);
    root->addLayout(headerRow);

    auto* note = new QLabel("ℹ️  Admin access required. Deactivated users cannot log in.");
    note->setStyleSheet(
        "color: #718096; font-size: 12px; "
        "background: #fffbeb; border-radius: 6px; padding: 8px 12px;");
    root->addWidget(note);

    m_table = new QTableWidget;
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({"ID", "Username", "Active", "Admin", "Created At", "Actions"});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setColumnWidth(0, 45);
    m_table->setColumnWidth(2, 70);
    m_table->setColumnWidth(3, 70);
    m_table->setColumnWidth(4, 160);
    m_table->setColumnWidth(5, 260);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    root->addWidget(m_table);

    connect(addBtn, &QPushButton::clicked, this, &UsersWidget::onAdd);
}

void UsersWidget::refresh() { loadData(); }

void UsersWidget::loadData() {
    QList<User> users = Database::instance().listUsers();
    m_table->setRowCount(0);
    m_table->setRowCount(static_cast<int>(users.size()));
    for (int i = 0; i < users.size(); ++i) {
        setRow(i, users[i]);
    }
}

void UsersWidget::setRow(int row, const User& u) {  // NOLINT
    auto center = [](const QString& s) {
        auto* item = new QTableWidgetItem(s);
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    };

    auto* idItem = new QTableWidgetItem(QString::number(u.id));
    idItem->setData(Qt::UserRole, u.id);
    idItem->setTextAlignment(Qt::AlignCenter);
    m_table->setItem(row, 0, idItem);
    m_table->setItem(row, 1, new QTableWidgetItem(u.username));

    auto* activeItem = center(u.isActive ? "✓ Yes" : "✗ No");
    activeItem->setForeground(u.isActive ? QColor("#2f855a") : QColor("#e53e3e"));
    m_table->setItem(row, 2, activeItem);

    auto* adminItem = center(u.isAdmin ? "★ Yes" : "No");
    adminItem->setForeground(u.isAdmin ? QColor("#b7791f") : QColor("#718096"));
    m_table->setItem(row, 3, adminItem);

    m_table->setItem(row, 4, new QTableWidgetItem(u.createdAt.toString("dd MMM yyyy hh:mm")));

    // Action buttons
    auto* actWidget = new QWidget;
    auto* actLayout = new QHBoxLayout(actWidget);
    actLayout->setContentsMargins(4, 2, 4, 2);
    actLayout->setSpacing(4);

    bool isSelf = (u.id == m_currentUser.id);

    auto makeBtn = [](const QString& text, const QString& bg, const QString& hoverBg) -> QPushButton* {
        auto* btn = new QPushButton(text);
        btn->setFixedHeight(24);
        btn->setStyleSheet(QString("QPushButton { background-color: %1; color: white; border: none; "
                                   "border-radius: 3px; font-size: 11px; padding: 0 7px; }"
                                   "QPushButton:hover { background-color: %2; }")
                               .arg(bg)
                               .arg(hoverBg));
        return btn;
    };

    auto* editBtn = makeBtn("Edit", "#718096", "#4a5568");
    int uid = u.id;
    connect(editBtn, &QPushButton::clicked, this, [this, uid] {
        User user = Database::instance().getUserById(uid);
        UserDialog dlg(this, user);
        if (dlg.exec() == QDialog::Accepted) {
            bool ok =
                Database::instance().updateUser(uid, dlg.getUsername(), dlg.getPassword(), dlg.shouldUpdatePassword());
            if (!ok) {
                QMessageBox::critical(this, "Error", Database::instance().lastError());
            } else {
                // Handle admin promotion/demotion
                if (dlg.isAdmin() && !user.isAdmin) {
                    Database::instance().promoteUser(uid);
                } else if (!dlg.isAdmin() && user.isAdmin) {
                    Database::instance().demoteUser(uid);
                }
                loadData();
            }
        }
    });
    actLayout->addWidget(editBtn);

    if (!isSelf) {
        if (u.isActive) {
            auto* deactBtn = makeBtn("Deactivate", "#d97706", "#b45309");
            connect(deactBtn, &QPushButton::clicked, this, [this, uid] {
                auto ret = QMessageBox::question(this, "Deactivate User",
                                                 "Deactivate this user? They will not be able to log in.",
                                                 QMessageBox::Yes | QMessageBox::No);
                if (ret == QMessageBox::Yes) {
                    Database::instance().deactivateUser(uid);
                    loadData();
                }
            });
            actLayout->addWidget(deactBtn);
        } else {
            auto* actBtn = makeBtn("Activate", "#2f855a", "#276749");
            connect(actBtn, &QPushButton::clicked, this, [this, uid] {
                Database::instance().activateUser(uid);
                loadData();
            });
            actLayout->addWidget(actBtn);
        }

        if (u.isAdmin) {
            auto* demoteBtn = makeBtn("Demote", "#b7791f", "#975a16");
            connect(demoteBtn, &QPushButton::clicked, this, [this, uid] {
                auto ret = QMessageBox::question(this, "Demote User", "Remove admin privileges from this user?",
                                                 QMessageBox::Yes | QMessageBox::No);
                if (ret == QMessageBox::Yes) {
                    Database::instance().demoteUser(uid);
                    loadData();
                }
            });
            actLayout->addWidget(demoteBtn);
        } else {
            auto* promoteBtn = makeBtn("Promote", "#553c9a", "#44337a");
            connect(promoteBtn, &QPushButton::clicked, this, [this, uid] {
                auto ret = QMessageBox::question(this, "Promote User", "Grant admin privileges to this user?",
                                                 QMessageBox::Yes | QMessageBox::No);
                if (ret == QMessageBox::Yes) {
                    Database::instance().promoteUser(uid);
                    loadData();
                }
            });
            actLayout->addWidget(promoteBtn);
        }

        auto* delBtn = makeBtn("Delete", "#e53e3e", "#c53030");
        connect(delBtn, &QPushButton::clicked, this, [this, uid] {
            auto ret =
                QMessageBox::question(this, "Delete User", "Permanently delete this user? This cannot be undone.",
                                      QMessageBox::Yes | QMessageBox::No);
            if (ret == QMessageBox::Yes) {
                if (!Database::instance().deleteUser(uid)) {
                    QMessageBox::critical(this, "Error", Database::instance().lastError());
                } else {
                    loadData();
                }
            }
        });
        actLayout->addWidget(delBtn);
    } else {
        auto* selfNote = new QLabel("(current user)");
        selfNote->setStyleSheet("color: #a0aec0; font-size: 11px;");
        actLayout->addWidget(selfNote);
    }

    actLayout->addStretch();
    m_table->setCellWidget(row, 5, actWidget);
}

void UsersWidget::onAdd() {
    UserDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        if (dlg.getPassword().length() < 8) {
            QMessageBox::warning(this, "Validation", "Password must be at least 8 characters.");
            return;
        }
        bool ok = Database::instance().createUser(dlg.getUsername(), dlg.getPassword(), dlg.isAdmin());
        if (!ok) {
            QMessageBox::critical(this, "Error", Database::instance().lastError());
        } else {
            loadData();
        }
    }
}
