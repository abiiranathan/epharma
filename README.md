# Tella POS

A desktop point-of-sale system built with **C++ / Qt 6** and **SQLite**. Designed for small to medium businesses — handles sales, stock management, invoicing, and reporting in a single offline application.

![Qt](https://img.shields.io/badge/Qt-6-41CD52?logo=qt) ![C++](https://img.shields.io/badge/C++-17-00599C?logo=cplusplus) ![SQLite](https://img.shields.io/badge/SQLite-3-003B57?logo=sqlite)

---

## Features

- **Point of Sale** — barcode scanning, product search, quantity editing, and one-click transaction saving
- **Inventory** — paginated product list with CSV import, expiry date tracking, and low-stock indicators
- **Invoices** — supplier invoice management with line-item stock-in recording
- **Transactions** — full transaction history with receipt printing and 1-hour cancellation window
- **Reports & Analytics**
  - Dashboard with daily/weekly/monthly/annual summaries and bar charts
  - Detailed sales report with income trend, profit-by-period, and top-products charts
  - Stock card — daily opening, in, out, reversal, and closing balances
- **User Management** — multi-user with admin/pharmacist roles, activation/deactivation


## Requirements

| Dependency   | Version                                             |
| ------------ | --------------------------------------------------- |
| Qt           | 6.x (Core, Gui, Widgets, Sql, Charts, PrintSupport) |
| CMake        | 3.30+                                               |
| C++ compiler | C++17 (GCC, Clang, MSVC)                            |
| SQLite       | bundled via Qt                                      |

## Building

```bash
git clone https://github.com/abiiranathan/tella-pos.git
cd tella-pos

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The compiled binary is at `build/tella`.

## Running

```bash
./build/tella
```

On first launch the database is created at:

| Platform | Path                                               |
| -------- | -------------------------------------------------- |
| Linux    | `~/.local/share/Tella POS/Tella POS/tella.db`      |
| macOS    | `~/Library/Application Support/Tella POS/tella.db` |
| Windows  | `%APPDATA%\Tella POS\tella.db`                     |

Default credentials: **admin / admin123** — change immediately after first login.

## Seeding Demo Data

Two SQL seed files are provided for development and demo purposes:

```bash
DB=~/.local/share/Tella\ POS/Tella\ POS/tella.db

# 103 real pharmacy products
sqlite3 "$DB" < seed_products.sql

# 918 transactions across 6 months (Oct 2025 → Apr 2026)
sqlite3 "$DB" < seed_transactions.sql
```

## Project Structure

```
epharma/
├── CMakeLists.txt
├── resources/
│   ├── resources.qrc
│   └── style.qss
└── src/
    ├── main.cpp
    ├── database.{hpp,cpp}        # All DB access — singleton, SQLite via Qt Sql
    ├── models.{hpp,cpp}          # Plain structs + JSON serialisation
    ├── loginwindow.{hpp,cpp}
    ├── mainwindow.{hpp,cpp}      # Shell with sidebar navigation
    ├── poswidget.{hpp,cpp}       # POS screen + barcode event filter
    ├── productswidget.{hpp,cpp}  # Inventory CRUD + CSV import
    ├── invoiceswidget.{hpp,cpp}  # Invoices + stock-in
    ├── transactionswidget.{hpp,cpp}
    ├── reportswidget.{hpp,cpp}   # Charts (QtCharts) + stock card
    └── userswidget.{hpp,cpp}
```

## Architecture Notes

- **Single SQLite file** with WAL mode and foreign keys enabled
- **Transactions stored as JSON** arrays in the `transactions.items` column — queried with SQLite's `json_each()` for reporting
- **Stock balances** maintained as daily snapshots in `stock_balances` for the stock card report
- **Barcode scanning** handled via a Qt application-level event filter that buffers rapid keystrokes into a barcode string

## License
Tella POS needs a commercial license and cannot be used for free. Please contact us for more information.