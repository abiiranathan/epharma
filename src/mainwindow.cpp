#include "mainwindow.hpp"
#include "invoiceswidget.hpp"
#include "poswidget.hpp"
#include "productswidget.hpp"
#include "reportswidget.hpp"
#include "transactionswidget.hpp"
#include "userswidget.hpp"

#include <QApplication>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QScreen>
#include <QVBoxLayout>

MainWindow::MainWindow(const User& user, QWidget* parent) : QMainWindow(parent), m_currentUser(user) {
    setWindowTitle("Tella — Point of Sale");
    setMinimumSize(1200, 780);

    if (QScreen* s = QApplication::primaryScreen()) {
        QRect sg = s->availableGeometry();
        resize(qMin(1400, sg.width() - 60), qMin(860, sg.height() - 60));
        move(sg.center() - rect().center());
    }

    setupUi();
}

void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ---- Sidebar ----
    auto* sidebar = new QWidget;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(210);
    auto* sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(0);

    // App title
    auto* titleWidget = new QWidget;
    titleWidget->setStyleSheet("background-color: #152d4a;");
    auto* titleLayout = new QVBoxLayout(titleWidget);
    titleLayout->setContentsMargins(16, 20, 16, 16);
    auto* appTitle = new QLabel("Tella POS");
    appTitle->setStyleSheet("color: #ffffff; font-size: 17px; font-weight: 700;");
    auto* appSub = new QLabel("Point of Sale System");
    appSub->setStyleSheet("color: #7aa3c8; font-size: 11px;");
    titleLayout->addWidget(appTitle);
    titleLayout->addWidget(appSub);
    sideLayout->addWidget(titleWidget);

    // Nav section label
    auto* navLabel = new QLabel("NAVIGATION");
    navLabel->setObjectName("sectionLabel");

    // Add margin to separate from title
    navLabel->setContentsMargins(12, 16, 0, 4);
    sideLayout->addWidget(navLabel);

    // Nav buttons
    addNavButton(sidebar, sideLayout, "🛒", "Point of Sale", 0);
    addNavButton(sidebar, sideLayout, "📦", "Inventory", 1);
    addNavButton(sidebar, sideLayout, "📄", "Invoices", 2);
    addNavButton(sidebar, sideLayout, "🧾", "Transactions", 3);
    addNavButton(sidebar, sideLayout, "📊", "Reports", 4);

    if (m_currentUser.isAdmin) {
        auto* adminLabel = new QLabel("ADMIN");
        adminLabel->setObjectName("sectionLabel");
        sideLayout->addWidget(adminLabel);
        // Add margin top to separate from above section
        adminLabel->setContentsMargins(0, 12, 0, 4);
        addNavButton(sidebar, sideLayout, "👥", "Users", 5);
    }

    sideLayout->addStretch();

    // User info at bottom
    auto* userWidget = new QWidget;
    userWidget->setStyleSheet("background-color: #152d4a; border-top: 1px solid #2d5a8e;");
    auto* userLayout = new QVBoxLayout(userWidget);
    userLayout->setContentsMargins(16, 12, 16, 12);
    m_userLabel = new QLabel(QString("👤 %1").arg(m_currentUser.username));
    m_userLabel->setStyleSheet("color: #cdd9e8; font-size: 13px; font-weight: 500;");
    auto* roleLabel = new QLabel(m_currentUser.isAdmin ? "Administrator" : "Pharmacist");
    roleLabel->setStyleSheet("color: #7aa3c8; font-size: 11px;");
    auto* logoutBtn = new QPushButton("Sign Out");
    logoutBtn->setObjectName("secondaryBtn");
    logoutBtn->setFixedHeight(30);
    logoutBtn->setStyleSheet(
        "QPushButton { background-color: #2d5a8e; color: #cdd9e8; "
        "border: none; border-radius: 4px; font-size: 12px; padding: 4px 12px; }"
        "QPushButton:hover { background-color: #3a7bd5; }");
    userLayout->addWidget(m_userLabel);
    userLayout->addWidget(roleLabel);
    userLayout->addWidget(logoutBtn);
    sideLayout->addWidget(userWidget);

    mainLayout->addWidget(sidebar);

    // ---- Content stack ----
    m_stack = new QStackedWidget;
    m_stack->setObjectName("contentStack");

    m_posWidget = new POSWidget(m_currentUser);
    m_productsWidget = new ProductsWidget(m_currentUser);
    m_invoicesWidget = new InvoicesWidget(m_currentUser);
    m_transactionsWidget = new TransactionsWidget(m_currentUser);
    m_reportsWidget = new ReportsWidget(m_currentUser);
    m_usersWidget = new UsersWidget(m_currentUser);

    m_stack->addWidget(m_posWidget);           // 0
    m_stack->addWidget(m_productsWidget);      // 1
    m_stack->addWidget(m_invoicesWidget);      // 2
    m_stack->addWidget(m_transactionsWidget);  // 3
    m_stack->addWidget(m_reportsWidget);       // 4
    m_stack->addWidget(m_usersWidget);         // 5

    mainLayout->addWidget(m_stack);

    // Status bar
    statusBar()->showMessage(QString("Logged in as: %1  |  Tella POS v1.0").arg(m_currentUser.username));

    // Connect signals
    connect(logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogout);

    // Activate POS by default
    if (!m_navBtns.isEmpty()) {
        m_navBtns[0]->setChecked(true);
    }
    switchPage(0);
}

void MainWindow::addNavButton(QWidget* /*w*/, QVBoxLayout* layout, const QString& icon, const QString& text,
                              int index) {
    auto* btn = new QPushButton(icon + "  " + text);
    btn->setCheckable(true);
    btn->setAutoExclusive(false);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(
        "QPushButton{ text-align: left; padding: 10px 16px; font-size: 14px; color: #082f5d; }"
        "QPushButton:hover { background-color: #2d5a8e; color: #e9eef4; }"
        "QPushButton:checked { background-color: #3a7bd5; color: white; }");

    int idx = index;
    connect(btn, &QPushButton::clicked, this, [this, idx] { switchPage(idx); });
    layout->addWidget(btn);
    m_navBtns.append(btn);
}

void MainWindow::switchPage(int index) {
    m_stack->setCurrentIndex(index);
    for (int i = 0; i < m_navBtns.size(); ++i) {
        m_navBtns[i]->setChecked(i == index);
    }

    // Refresh widgets when switched to
    if (index == 1) {
        m_productsWidget->refresh();
    } else if (index == 2) {
        m_invoicesWidget->refresh();
    } else if (index == 3) {
        m_transactionsWidget->refresh();
    } else if (index == 4) {
        m_reportsWidget->refresh();
    } else if (index == 5) {
        m_usersWidget->refresh();
    }
}

void MainWindow::onLogout() {
    auto ret = QMessageBox::question(this, "Logout", "Are you sure you want to sign out?",
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::NoButton);

    if (ret == QMessageBox::Yes) {
        close();
        qApp->exit(0);
    }
}
