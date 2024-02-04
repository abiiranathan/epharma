#include "epharma.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <vector>

static std::string databaseName = ":memory:";

void setDatabase(const std::string& dbname) {
    databaseName = dbname;
}

int compute_days_to_expiry(const std::string& expiry_date, const date& now) {
    if (!validate_expiry_date(expiry_date)) {
        return -1;
    }

    using seconds = std::chrono::seconds;
    using system_clock = std::chrono::system_clock;

    date d = dateFromString(expiry_date);            // convert string to a date
    date today = dateFromString(dateToString(now));  // truncate properly for cmp

    auto duration = std::chrono::duration_cast<seconds>(d - today).count();
    return duration / (60 * 60 * 24);
}

std::string dateTimeToString(date_time time) {
    std::time_t now_c = std::chrono::system_clock::to_time_t(time);
    auto tm = std::localtime(&now_c);
    char buffer[32];
    std::strftime(buffer, 32, "%Y-%m-%d %H:%M:%S", tm);
    return std::string(buffer);
}

date_time dateTimeFromString(const std::string& s) {
    std::tm timeDate = {};
    std::istringstream ss(s);
    ss >> std::get_time(&timeDate, "%Y-%m-%d %H:%M:%S");
    return std::chrono::system_clock::from_time_t(mktime(&timeDate));
}

// Convert a date to a string in the format "YYYY-MM-DD"
std::string dateToString(date d) {
    std::time_t now_c = std::chrono::system_clock::to_time_t(d);
    auto tm = std::localtime(&now_c);
    char buffer[32];
    std::strftime(buffer, 32, "%Y-%m-%d", tm);
    return std::string(buffer);
}

// Convert a string in the format "YYYY-MM-DD" to a date
date dateFromString(const std::string& s) {
    std::tm timeDate = {};
    std::istringstream ss(s);
    ss >> std::get_time(&timeDate, "%Y-%m-%d");
    return std::chrono::system_clock::from_time_t(mktime(&timeDate));
}

bool is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

bool validate_expiry_date(const std::string& date) {
    std::tm tm = {};
    std::istringstream ss(date);

    // Parse the date string into a tm struct
    if (ss >> std::get_time(&tm, "%Y-%m-%d")) {
        // Check if the parsed values are within valid ranges
        int year = tm.tm_year + 1900;  // tm_year is years since 1900
        int month = tm.tm_mon + 1;     // tm_mon is months since January (0-based)

        // Validate year, month, and day
        if (year >= 1900 && month >= 1 && month <= 12 && tm.tm_mday >= 1 && tm.tm_mday <= 31 &&
            (month != 2 || (tm.tm_mday <= 29 && is_leap_year(year)))) {
            return true;  // All components are within valid ranges
        }
    }

    // Invalid date format, out-of-range components
    return false;
}

// Epharma
Epharma::Epharma() {
    // open the database
    int rc = sqlite3_open(databaseName.c_str(), &db);
    if (rc) {
        std::cerr << "Error opening SQLite3 database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        exit(1);
    }

    // create the tables
    try {
        create_tables();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        sqlite3_close(db);
        exit(1);
    }
}

Epharma::~Epharma() {
    // close the database
    sqlite3_close(db);
}

// Initialize the static const members
const std::string InventoryItem::TABLE_NAME = "inventory_items";
const std::string SalesItem::TABLE_NAME = "sales_items";
const std::string User::TABLE_NAME = "users";
const std::string StockIn::TABLE_NAME = "stock_ins";

// create the inventory table
void Epharma::create_tables() {
    std::string createInventoryTable =
        "CREATE TABLE IF NOT EXISTS " + InventoryItem::TABLE_NAME +
        " ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL,"
        "brand TEXT NOT NULL, "
        "quantity INTEGER NOT NULL, "
        "cost_price REAL NOT NULL, "
        "selling_price REAL NOT NULL, "
        "expiry_date TEXT NULL, "
        "created_at Timestamp DATETIME DEFAULT CURRENT_TIMESTAMP NOT NULL, "
        "barcode TEXT NULL UNIQUE, "
        "UNIQUE(name, brand) "
        ");";
    execute(createInventoryTable);

    // create the sales table
    std::string createSalesTable =
        "CREATE TABLE IF NOT EXISTS " + SalesItem::TABLE_NAME +
        " ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "item_id INTEGER NOT NULL, "
        "item_name TEXT NOT NULL, "
        "quantity INTEGER NOT NULL, "
        "cost_price REAL NOT NULL, "
        "selling_price REAL NOT NULL, "
        "created_at Timestamp DATETIME DEFAULT CURRENT_TIMESTAMP NOT NULL"
        ");";
    execute(createSalesTable);

    // create the user table
    std::string createUserTable =
        "CREATE TABLE IF NOT EXISTS " + User::TABLE_NAME +
        " ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "username TEXT NOT NULL UNIQUE, "
        "password TEXT NOT NULL, "
        "created_at Timestamp DATETIME DEFAULT CURRENT_TIMESTAMP NOT NULL)";
    execute(createUserTable);

    // create the stock in table
    std::string createStockInTable =
        "CREATE TABLE IF NOT EXISTS " + StockIn::TABLE_NAME +
        " (id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "item_id INTEGER NOT NULL, "
        "quantity INTEGER NOT NULL, "
        "invoice_no TEXT NOT NULL, "
        "batch_no TEXT NOT NULL, "
        "expiry_date TEXT NOT NULL, "
        "created_at Timestamp DATETIME DEFAULT CURRENT_TIMESTAMP NOT NULL)";

    execute(createStockInTable);

    // create a view to get the sales report
    std::string createSalesReportView =
        "CREATE VIEW IF NOT EXISTS sales_reports AS "
        "SELECT strftime('%Y-%m-%d', s.created_at) AS sale_date, "
        "i.name AS item_name, i.brand AS item_brand, "
        "SUM(s.quantity) AS quantity, SUM(s.cost_price * s.quantity) AS total_cost_price, "
        "SUM(s.selling_price * s.quantity) AS total_selling_price, "
        "SUM(s.selling_price - s.cost_price) * s.quantity AS total_profit "
        "FROM sales_items s "
        "JOIN inventory_items i ON s.item_id = i.id "
        "GROUP BY sale_date, item_name, item_brand "
        "ORDER BY sale_date DESC;";

    execute(createSalesReportView);
}

long long Epharma::execute(const std::string& sql) {
    char* errMsg;
    int rc = sqlite3_exec(db, sql.c_str(), NULL, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::string error = "Error executing SQL: " + std::string(errMsg);
        // print the query that caused the error
        std::cerr << "SQL: " << sql << std::endl;
        sqlite3_free(errMsg);
        throw std::runtime_error(error);
    }

    sqlite3_free(errMsg);
    sqlite3_int64 lastId = sqlite3_last_insert_rowid(db);
    return lastId;
}

long long Epharma::execute_stmt(sqlite3_stmt* stmt) {
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::string error = "Error executing SQL: " + std::string(sqlite3_errmsg(db));
        throw std::runtime_error(error);
    }
    sqlite3_finalize(stmt);
    sqlite3_int64 lastId = sqlite3_last_insert_rowid(db);
    return lastId;
}

// Implement the InventoryItem methods
double SalesItem::total_cost_price() const {
    return quantity * cost_price;
}

double SalesItem::total_selling_price() const {
    return quantity * selling_price;
}

double SalesItem::total_profit() const {
    return total_selling_price() - total_cost_price();
}

// create a new inventory item
long long Epharma::create_inventory_item(const InventoryItem& item) {
    // use a prepared statement to insert the inventory item
    std::string sql = "INSERT INTO " + InventoryItem::TABLE_NAME +
                      " (id, name, brand, quantity, cost_price, selling_price, expiry_date) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing SQL: " << sqlite3_errmsg(db) << std::endl;
        throw std::runtime_error("create_inventory_item(): Error preparing SQL");
    }

    // bind the values to the prepared statement
    if (item.id > 0) {
        sqlite3_bind_int(stmt, 1, item.id);
    } else {
        sqlite3_bind_null(stmt, 1);
    }

    sqlite3_bind_text(stmt, 2, item.name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, item.brand.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, item.quantity);
    sqlite3_bind_double(stmt, 5, item.cost_price);
    sqlite3_bind_double(stmt, 6, item.selling_price);
    sqlite3_bind_text(stmt, 7, item.expiry_date.c_str(), -1, SQLITE_STATIC);

    // execute the prepared statement
    return execute_stmt(stmt);
}

std::vector<long long> Epharma::insert_inventory_items(const std::vector<InventoryItem>& items) {
    std::vector<long long> insertedIds;
    insertedIds.reserve(items.size());

    for (const auto& item : items) {
        insertedIds.push_back(create_inventory_item(item));
    }
    return insertedIds;
}

// update an existing inventory item
void Epharma::update_inventory_item(const InventoryItem& item) {
    // assert that the item has an id
    if (item.id <= 0) {
        throw std::invalid_argument("Inventory item must have an id");
    }

    // use a prepared statement to update the inventory item
    std::string sql = "UPDATE " + InventoryItem::TABLE_NAME +
                      " SET name = ?, brand = ?, quantity = ?, cost_price = ?, selling_price = ?, "
                      "expiry_date = ? "
                      "WHERE id = ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing SQL: " << sqlite3_errmsg(db) << std::endl;
        throw std::runtime_error("update_inventory_item(): Error preparing SQL");
    }

    // bind the values to the prepared statement
    sqlite3_bind_text(stmt, 1, item.name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, item.brand.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, item.quantity);
    sqlite3_bind_double(stmt, 4, item.cost_price);
    sqlite3_bind_double(stmt, 5, item.selling_price);
    sqlite3_bind_text(stmt, 6, item.expiry_date.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 7, item.id);

    // execute the prepared statement
    execute_stmt(stmt);
}

// delete an existing inventory item
void Epharma::delete_inventory_item(int id) {
    std::string sql =
        "DELETE FROM " + InventoryItem::TABLE_NAME + " WHERE id = " + std::to_string(id) + ";";
    execute(sql);
}

static void scanInventoryItem(InventoryItem& item, sqlite3_stmt* stmt) {
    item.id = sqlite3_column_int(stmt, 0);
    item.name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
    item.brand = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
    item.quantity = sqlite3_column_int(stmt, 3);
    item.cost_price = sqlite3_column_double(stmt, 4);
    item.selling_price = sqlite3_column_double(stmt, 5);

    const unsigned char* expiry_date = sqlite3_column_text(stmt, 6);
    if (expiry_date != nullptr) {
        item.expiry_date = std::string(reinterpret_cast<const char*>(expiry_date));
    }

    std::string created_at =
        std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
    item.created_at = dateTimeFromString(created_at);

    const unsigned char* barcode = sqlite3_column_text(stmt, 8);
    if (barcode != nullptr) {
        item.barcode = std::string(reinterpret_cast<const char*>(barcode));
    }
}

// get all inventory items
std::vector<InventoryItem> Epharma::get_inventory_items() {
    std::vector<InventoryItem> items;
    std::string sql = "SELECT * FROM " + InventoryItem::TABLE_NAME + ";";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing SQL: " << sqlite3_errmsg(db) << std::endl;
        return items;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        InventoryItem item;
        scanInventoryItem(item, stmt);
        items.push_back(item);
    }

    sqlite3_finalize(stmt);
    return items;
}

// get a single inventory item
InventoryItem Epharma::get_inventory_item(int id) {
    std::string sql = "SELECT * FROM " + InventoryItem::TABLE_NAME + " WHERE id = ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    sqlite3_bind_int(stmt, 1, id);

    InventoryItem item;
    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        scanInventoryItem(item, stmt);
    } else {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    return item;
}

void Epharma::update_barcode(int item_id, const std::string& barcode) {
    std::string sql = "UPDATE " + InventoryItem::TABLE_NAME + " SET barcode = ? WHERE id = ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    sqlite3_bind_text(stmt, 1, barcode.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, item_id);

    execute_stmt(stmt);
}

// get inventory item by barcode
InventoryItem Epharma::get_inventory_item_by_barcode(const std::string& barcode) {
    std::string sql = "SELECT * FROM " + InventoryItem::TABLE_NAME + " WHERE barcode = ? LIMIT 1";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    sqlite3_bind_text(stmt, 1, barcode.c_str(), -1, SQLITE_STATIC);

    InventoryItem item;
    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        scanInventoryItem(item, stmt);
    } else {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    return item;
}

// create a new sales item
long long Epharma::create_sales_item(const SalesItem& item, bool transaction) {
    // use a prepared statement to insert the sales item
    std::string sql = "INSERT INTO " + SalesItem::TABLE_NAME +
                      " (id, item_id, item_name, quantity, cost_price, selling_price) "
                      "VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing SQL: " << sqlite3_errmsg(db) << std::endl;
        throw std::runtime_error("create_sales_item(): Error preparing SQL");
    }

    // bind the values to the prepared statement
    if (item.id > 0) {
        sqlite3_bind_int(stmt, 1, item.id);
    } else {
        sqlite3_bind_null(stmt, 1);
    }

    sqlite3_bind_int(stmt, 2, item.item_id);
    sqlite3_bind_text(stmt, 3, item.item_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, item.quantity);
    sqlite3_bind_double(stmt, 5, item.cost_price);
    sqlite3_bind_double(stmt, 6, item.selling_price);

    // execute the prepared statement
    if (transaction) {
        execute("BEGIN TRANSACTION");
    }
    try {
        auto itemId = execute_stmt(stmt);

        // update the quantity of the inventory item
        InventoryItem inventoryItem = get_inventory_item(item.item_id);
        if (transaction) {
            execute("COMMIT");
        }
        return itemId;
    } catch (std::runtime_error& e) {
        if (transaction) {
            execute("ROLLBACK");
        }
        throw e;
    }
}

std::vector<long long> Epharma::create_sales_items(const std::vector<SalesItem>& items) {
    std::vector<long long> insertedIds(items.size());

    execute("BEGIN TRANSACTION");
    try {
        for (const auto& item : items) {
            insertedIds.push_back(create_sales_item(item, false));
        }
    } catch (std::runtime_error& e) {
        execute("ROLLBACK");
        throw e;
    }

    execute("COMMIT");
    return insertedIds;
}

// search for an inventory item by name
std::vector<InventoryItem> Epharma::search_inventory_items(const std::string& name) {
    std::vector<InventoryItem> items;
    std::string sql = "SELECT * FROM " + InventoryItem::TABLE_NAME + " WHERE name LIKE ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing SQL: " << sqlite3_errmsg(db) << std::endl;
        return items;
    }

    // bind the values to the prepared statement
    const std::string& param = "%" + name + "%";

    sqlite3_bind_text(stmt, 1, param.c_str(), -1, SQLITE_STATIC);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        InventoryItem item;
        item.id = sqlite3_column_int(stmt, 0);
        item.name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        item.brand = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        item.quantity = sqlite3_column_int(stmt, 3);
        item.cost_price = sqlite3_column_double(stmt, 4);
        item.selling_price = sqlite3_column_double(stmt, 5);
        item.expiry_date = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
        item.created_at = dateTimeFromString(
            std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7))));
        items.push_back(item);
    }

    sqlite3_finalize(stmt);
    return items;
}

// get all sales items
std::vector<SalesItem> Epharma::get_sales_items() {
    std::vector<SalesItem> items;
    std::string sql = "SELECT * FROM " + SalesItem::TABLE_NAME + ";";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing SQL: " << sqlite3_errmsg(db) << std::endl;
        return items;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        SalesItem item;
        item.id = sqlite3_column_int(stmt, 0);
        item.item_id = sqlite3_column_int(stmt, 1);
        item.item_name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        item.quantity = sqlite3_column_int(stmt, 3);
        item.cost_price = sqlite3_column_double(stmt, 4);
        item.selling_price = sqlite3_column_double(stmt, 5);
        auto created_at = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
        item.created_at = dateTimeFromString(created_at);
        items.push_back(item);
    }

    sqlite3_finalize(stmt);
    return items;
}

// get a single sales item
SalesItem Epharma::get_sales_item(int id) {
    SalesItem item;
    std::string sql =
        "SELECT * FROM " + SalesItem::TABLE_NAME + " WHERE id = " + std::to_string(id) + ";";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing SQL: " << sqlite3_errmsg(db) << std::endl;
        return item;
    }

    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        item.id = sqlite3_column_int(stmt, 0);
        item.item_id = sqlite3_column_int(stmt, 1);
        item.item_name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        item.quantity = sqlite3_column_int(stmt, 3);
        item.cost_price = sqlite3_column_double(stmt, 4);
        item.selling_price = sqlite3_column_double(stmt, 5);
        item.created_at = dateTimeFromString(
            std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6))));
    }

    sqlite3_finalize(stmt);
    return item;
}

// search sales items by name
std::vector<SalesItem> Epharma::search_sales_items(const std::string& name) {
    std::vector<SalesItem> items;
    std::string sql = "SELECT * FROM " + SalesItem::TABLE_NAME + " WHERE item_name LIKE ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing SQL: " << sqlite3_errmsg(db) << std::endl;
        return items;
    }

    // bind the values to the prepared statement

    const std::string& param = "%" + name + "%";

    sqlite3_bind_text(stmt, 1, param.c_str(), -1, SQLITE_STATIC);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        SalesItem item;
        item.id = sqlite3_column_int(stmt, 0);
        item.item_id = sqlite3_column_int(stmt, 1);
        item.item_name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        item.quantity = sqlite3_column_int(stmt, 3);
        item.cost_price = sqlite3_column_double(stmt, 4);
        item.selling_price = sqlite3_column_double(stmt, 5);
        item.created_at = dateTimeFromString(
            std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6))));
        items.push_back(item);
    }

    sqlite3_finalize(stmt);
    return items;
}

// create a new user
long long Epharma::create_user(const User& user) {
    // use a prepared statement to insert the user
    std::string sql =
        "INSERT INTO " + User::TABLE_NAME + " (id, username, password) VALUES (?, ?, ?);";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing SQL: " << sqlite3_errmsg(db) << std::endl;
        throw std::runtime_error("create_user(): Error preparing SQL");
    }

    // bind the values to the prepared statement
    if (user.id > 0) {
        sqlite3_bind_int(stmt, 1, user.id);
    } else {
        sqlite3_bind_null(stmt, 1);
    }

    sqlite3_bind_text(stmt, 2, user.username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, user.password.c_str(), -1, SQLITE_STATIC);

    // execute the prepared statement
    return execute_stmt(stmt);
}

// Update user account
void Epharma::update_user(const User& user) {
    // assert that the item has an id
    if (user.id <= 0) {
        throw std::invalid_argument("User must have a valid id");
    }

    // use a prepared statement to update the inventory item
    std::string sql = "UPDATE " + User::TABLE_NAME + " SET username = ?, password = ? WHERE id=?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing SQL: " << sqlite3_errmsg(db) << std::endl;
        throw std::runtime_error("update_user(): Error preparing SQL");
    }

    // bind the values to the prepared statement
    sqlite3_bind_text(stmt, 1, user.username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user.password.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, user.id);

    // execute the prepared statement
    execute_stmt(stmt);
}

// get all users
std::vector<User> Epharma::get_users() {
    std::vector<User> users;
    std::string sql = "SELECT * FROM " + User::TABLE_NAME + ";";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing SQL: " << sqlite3_errmsg(db) << std::endl;
        return users;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        User user;
        user.id = sqlite3_column_int(stmt, 0);
        user.username = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        user.password = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        user.created_at = dateTimeFromString(
            std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3))));
        users.push_back(user);
    }

    sqlite3_finalize(stmt);
    return users;
}

// get a single user
User Epharma::get_user(int id) {
    User user;
    std::string sql =
        "SELECT * FROM " + User::TABLE_NAME + " WHERE id = " + std::to_string(id) + ";";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing SQL: " << sqlite3_errmsg(db) << std::endl;
        return user;
    }

    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        user.id = sqlite3_column_int(stmt, 0);
        user.username = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        user.password = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        user.created_at = dateTimeFromString(
            std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3))));
    }

    sqlite3_finalize(stmt);
    return user;
}

// get a single user by username
User Epharma::get_user_by_username(const std::string& username) {
    // use a prepared statement to get the user by username
    User user;

    std::string sql = "SELECT * FROM " + User::TABLE_NAME + " WHERE username = ? LIMIT 1;";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing SQL: " << sqlite3_errmsg(db) << std::endl;
        return user;
    }

    // bind the values to the prepared statement
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        user.id = sqlite3_column_int(stmt, 0);
        user.username = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        user.password = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        user.created_at = dateTimeFromString(
            std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3))));
    }

    sqlite3_finalize(stmt);
    return user;
}

// delete a user
void Epharma::delete_user(int id) {
    std::string sql = "DELETE FROM " + User::TABLE_NAME + " WHERE id = " + std::to_string(id) + ";";
    execute(sql);
}

// check if a user exists
bool Epharma::user_exists(const std::string& username) {
    User user = get_user_by_username(username);
    return user.username == username && user.id > 0;
}

// authenticate a user
bool Epharma::authenticate_user(const std::string& username, const std::string& password) {
    User user = get_user_by_username(username);
    return user.password == password;
}

// Overload the << operator for InventoryItem
std::ostream& operator<<(std::ostream& os, const InventoryItem& item) {
    os << "ID: " << item.id << ", Name: " << item.name << ", Brand: " << item.brand
       << ", Quantity: " << item.quantity << ", Cost Price: " << item.cost_price
       << ", Selling Price: " << item.selling_price << ", Expiry Date: " << item.expiry_date
       << ", Created At: " << dateTimeToString(item.created_at);
    return os;
}

// Overload the << operator for SalesItem
std::ostream& operator<<(std::ostream& os, const SalesItem& item) {
    os << "ID: " << item.id << ", Item ID: " << item.item_id << ", Item Name: " << item.item_name
       << ", Quantity: " << item.quantity << ", Cost Price: " << item.cost_price
       << ", Selling Price: " << item.selling_price
       << ", Created At: " << dateTimeToString(item.created_at);
    return os;
}

// overload the << operator for User
std::ostream& operator<<(std::ostream& os, const User& user) {
    os << "ID: " << user.id << ", Username: " << user.username
       << ", Created At: " << dateTimeToString(user.created_at);
    return os;
}

// ======================================== StockIn ========================================
long long Epharma::create_stock_in(const StockIn& stockin, bool transaction) {
    // use a prepared statement to insert the stock in item
    std::string sql = "INSERT INTO " + StockIn::TABLE_NAME +
                      " (id, item_id, quantity, invoice_no, batch_no, expiry_date) "
                      "VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing SQL: " << sqlite3_errmsg(db) << std::endl;
        throw std::runtime_error("create_stock_in(): Error preparing SQL");
    }

    if (stockin.id > 0) {
        sqlite3_bind_int(stmt, 1, stockin.id);
    } else {
        sqlite3_bind_null(stmt, 1);
    }

    // bind the values to the prepared statement
    sqlite3_bind_int(stmt, 2, stockin.item_id);
    sqlite3_bind_int(stmt, 3, stockin.quantity);
    sqlite3_bind_text(stmt, 4, stockin.invoice_no.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, stockin.batch_no.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, stockin.expiry_date.c_str(), -1, SQLITE_STATIC);

    // execute the prepared statement
    if (transaction) {
        execute("BEGIN TRANSACTION");
    }

    try {
        long long insertedId = execute_stmt(stmt);
        // update the quantity of the inventory item
        InventoryItem item = get_inventory_item(stockin.item_id);
        item.quantity += stockin.quantity;
        update_inventory_item(item);

        if (transaction) {
            execute("COMMIT");
        }
        return insertedId;
    } catch (std::runtime_error& e) {
        if (transaction) {
            execute("ROLLBACK");
        }
        throw e;
    }
}

// Insert multiple stock in items. Returns vector of inserted item ids.
std::vector<long long> Epharma::create_stock_in_items(const std::vector<StockIn>& items) {
    std::vector<long long> insertedIds;
    insertedIds.reserve(items.size());

    execute("BEGIN TRANSACTION");
    try {
        for (const auto& item : items) {
            insertedIds.push_back(create_stock_in(item, false));
        }
    } catch (std::runtime_error& e) {
        execute("ROLLBACK");
        throw e;
    }
    execute("COMMIT");
    return insertedIds;
}

static void scanStockIn(StockIn& stockin, sqlite3_stmt* stmt) {
    stockin.id = sqlite3_column_int(stmt, 0);
    stockin.item_id = sqlite3_column_int(stmt, 1);
    stockin.quantity = sqlite3_column_int(stmt, 2);
    stockin.invoice_no = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
    stockin.batch_no = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
    stockin.expiry_date = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
    stockin.created_at = dateTimeFromString(
        std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6))));
}

// get all stock in items
std::vector<StockIn> Epharma::get_stock_in_items() {
    std::vector<StockIn> items;
    std::string sql = "SELECT * FROM " + StockIn::TABLE_NAME + " ORDER BY created_at DESC";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing SQL: " << sqlite3_errmsg(db) << std::endl;
        return items;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        StockIn stockin;
        scanStockIn(stockin, stmt);
        items.push_back(stockin);
    }

    sqlite3_finalize(stmt);
    return items;
}

// search for stock in items
std::vector<StockIn> Epharma::search_stock_in_items(const std::string& column,
                                                    const std::string& query) {
    // validate the column
    if (column != "invoice_no" && column != "batch_no") {
        throw std::invalid_argument(
            "Invalid column name. Must be one of 'invoice_no', "
            "'batch_no'");
    }

    std::vector<StockIn> items;
    std::string sql = "SELECT * FROM " + StockIn::TABLE_NAME + " WHERE " + column + " LIKE ?";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing SQL: " << sqlite3_errmsg(db) << std::endl;
        return items;
    }

    // bind the values to the prepared statement
    const std::string& param = "%" + query + "%";
    sqlite3_bind_text(stmt, 1, param.c_str(), -1, SQLITE_STATIC);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        StockIn stockin;
        scanStockIn(stockin, stmt);
        items.push_back(stockin);
    }

    sqlite3_finalize(stmt);
    return items;
}

// get a single stock in item
StockIn Epharma::get_stock_in_item(int id) {
    std::string sql = "SELECT * FROM " + StockIn::TABLE_NAME + " WHERE id = ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    sqlite3_bind_int(stmt, 1, id);

    StockIn stockin;
    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        scanStockIn(stockin, stmt);
    } else {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    return stockin;
}

// get stock in items by item id
std::vector<StockIn> Epharma::get_stock_in_items_by_item_id(int item_id) {
    std::vector<StockIn> items;
    std::string sql = "SELECT * FROM " + StockIn::TABLE_NAME +
                      " WHERE item_id = " + std::to_string(item_id) + " ORDER BY created_at DESC";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        StockIn stockin;
        scanStockIn(stockin, stmt);
        items.push_back(stockin);
    }

    sqlite3_finalize(stmt);
    return items;
}

// delete a stock in item
void Epharma::delete_stock_in_item(int id) {
    // update the inventory item quantity
    StockIn stockin = get_stock_in_item(id);
    InventoryItem item = get_inventory_item(stockin.item_id);

    // delete the stock in item in a transaction
    std::string sql =
        "DELETE FROM " + StockIn::TABLE_NAME + " WHERE id = " + std::to_string(id) + ";";

    execute("BEGIN TRANSACTION");
    try {
        execute(sql);
        item.quantity -= stockin.quantity;
        update_inventory_item(item);
    } catch (const std::exception& e) {
        execute("ROLLBACK");
        throw e;
    }
    execute("COMMIT");
}

std::vector<SalesReport> Epharma::get_sales_report(const std::string& start_date,
                                                   const std::string& end_date,
                                                   const std::string& item_name,
                                                   const std::string& item_brand) {
    std::vector<SalesReport> salesReports;

    std::string sql = "SELECT * FROM sales_reports WHERE sale_date BETWEEN ? AND ? ";
    if (!item_name.empty()) {
        sql += "AND item_name LIKE ? ";
    }

    if (!item_brand.empty()) {
        sql += "AND item_brand LIKE ? ";
    }

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    sqlite3_bind_text(stmt, 1, start_date.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, end_date.c_str(), -1, SQLITE_STATIC);

    if (!item_name.empty()) {
        const std::string& param = "%" + item_name + "%";
        sqlite3_bind_text(stmt, 3, param.c_str(), -1, SQLITE_STATIC);
    }

    if (!item_brand.empty()) {
        const std::string& param = "%" + item_brand + "%";
        sqlite3_bind_text(stmt, 4, param.c_str(), -1, SQLITE_STATIC);
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        SalesReport report;
        report.sale_date = dateTimeFromString(
            std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));

        report.item_name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        report.item_brand =
            std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        report.total_quantity_sold = sqlite3_column_int(stmt, 3);
        report.total_cost_price = sqlite3_column_double(stmt, 4);
        report.total_selling_price = sqlite3_column_double(stmt, 5);
        report.total_profit = sqlite3_column_double(stmt, 6);
        salesReports.push_back(report);
    }

    sqlite3_finalize(stmt);
    return salesReports;
}
