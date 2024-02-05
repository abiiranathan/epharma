#ifndef EPHARMA_H
#define EPHARMA_H

#include <sqlite3.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace epharma {
// Define the date_time type
using date_time = std::chrono::system_clock::time_point;

// Define a date type
using date = std::chrono::system_clock::time_point;

// Convert a date_time to a string in the format "YYYY-MM-DD HH:MM:SS"
std::string dateTimeToString(date_time time);

// Convert a string in the format "YYYY-MM-DD HH:MM:SS" to a date_time
date_time dateTimeFromString(const std::string& s);

// Convert a date to a string in the format "YYYY-MM-DD"
std::string dateToString(date d);

// Convert a string in the format "YYYY-MM-DD" to a date
date dateFromString(const std::string& s);

// Validate expiry date format
bool validate_expiry_date(const std::string& date);

// compute days to expire or since expiry
// If days are positive, the items is not yet expired.
// if negative, it's already expired or expiry date was not set.
// If the expiry date if not valid, this function will return -1.
int compute_days_to_expiry(const std::string& expiry_date,
                           const date& now = std::chrono::system_clock::now());

// Sets the sqlite3 database name
void setDatabase(const std::string& file);

// Returns the configured database name
// default is ":memory:"
const std::string getDatabaseName();

// Define the format for the date_time that can be used by
// QDateTime::fromString and QDateTime::toString
const std::string QDATETIME_FORMAT = "yyyy-MM-dd HH:mm:ss";

// Define the format for the date that can be used by
// QDate::fromString and QDate::toString
const std::string QDATE_FORMAT = "yyyy-MM-dd";

struct InventoryItem {
    static const std::string TABLE_NAME;  // table name, set by the application

    int id;                   // Primary key.
    std::string name;         // Generic name. (name, brand) must be unique
    std::string brand;        // Brand name.
    int quantity;             // Quantity in stock
    double cost_price;        // Cost price
    double selling_price;     // Selling price
    std::string expiry_date;  // Format: YYYY-MM-DD
    date_time created_at;     // Auto-created by the database
    std::string barcode;      // unique barcode

    // Returns true if item's expiry date is before today or to today.
    // It will also return true if expiry date is not a valid date.
    bool is_expired() { return compute_days_to_expiry(expiry_date) <= 0; }

    // overload the << operator for InventoryItem
    friend std::ostream& operator<<(std::ostream& os, const InventoryItem& item);
};

// A vector of inventory items
typedef std::vector<InventoryItem> InventoryItems;

struct SalesItem {
    static const std::string TABLE_NAME;

    int id;
    int item_id;            // id to inventory item
    std::string item_name;  // name inventory item
    int quantity;           // quantity sold
    double cost_price;      // cost price of item
    double selling_price;   // selling price of item
    date_time created_at;

    // return the total cost price of the sales item
    double total_cost_price() const;

    // return the total selling price of the sales item
    double total_selling_price() const;

    // return the total profit of the sales item
    double total_profit() const;

    // overload the << operator for SalesItem
    friend std::ostream& operator<<(std::ostream& os, const SalesItem& item);
};

// A vector of SaleItems
typedef std::vector<SalesItem> SaleItems;

struct User {
    static const std::string TABLE_NAME;

    int id;
    std::string username;  // unique username
    std::string password;  // A simple unhashed password
    date_time created_at;  // Timestamp

    // overload the << operator for User
    friend std::ostream& operator<<(std::ostream& os, const User& user);
};

// Holds data to be added to stock.
struct StockIn {
    static const std::string TABLE_NAME;

    int id;                   // PK for this stockin
    int item_id;              // Foreign Key to Inventory item
    int quantity;             // Quantity stocked.
    std::string invoice_no;   // Invoice number
    std::string batch_no;     // batch number;
    std::string expiry_date;  // Expiry date(YYYY-MMM-dd)
    date_time created_at;     // Created at timestamp
};

// daily, weekly, monthly, yearly sales report
// Generated from the view: sales_reports
struct SalesReport {
    date_time sale_date;
    std::string item_name;
    std::string item_brand;
    int total_quantity_sold;
    double total_cost_price;
    double total_selling_price;
    double total_profit;
};

class Epharma {
   private:
    // the SQLite3 database
    sqlite3* db;

    void create_tables() noexcept(false);

    // Returns the last insert rows id or 0
    // If none exists.
    long long execute(const std::string& sql) noexcept(false);
    long long execute_stmt(sqlite3_stmt* stmt) noexcept(false);

   public:
    // constructor
    Epharma() noexcept(false);

    // destructor
    ~Epharma();

    // ============= INVENTORY ITEMS ============================

    // create a new inventory item. Returns id of inserted item.
    long long create_inventory_item(const InventoryItem& item) noexcept(false);

    // insert_inventory_items. Returns a vector of insrted ids.
    std::vector<long long> insert_inventory_items(const std::vector<InventoryItem>& items) noexcept(
        false);

    // update an existing inventory item
    void update_inventory_item(const InventoryItem& item) noexcept(false);

    // delete an existing inventory item
    void delete_inventory_item(int id) noexcept(false);

    // get all inventory items.
    std::vector<InventoryItem> get_inventory_items() noexcept(false);

    // search for inventory items
    std::vector<InventoryItem> search_inventory_items(const std::string& query) noexcept(false);

    // get a single inventory item
    InventoryItem get_inventory_item(int id) noexcept(false);

    // The item_id is the id of the inventory item. Must already exist in the inventory
    void update_barcode(int item_id, const std::string& barcode) noexcept(false);

    // get inventory item by barcode
    InventoryItem get_inventory_item_by_barcode(const std::string& barcode) noexcept(false);

    // =============== SALES ITEMS ==============================
    // create a new sales item. Returns id of inserted item.
    long long create_sales_item(const SalesItem& item, bool transaction = true) noexcept(false);

    // Inserts multiple sales items. Returns vector of inserted item ids.
    std::vector<long long> create_sales_items(const std::vector<SalesItem>& items) noexcept(false);

    // get all sales items
    std::vector<SalesItem> get_sales_items() noexcept(false);

    // search for sales items
    std::vector<SalesItem> search_sales_items(const std::string& query) noexcept(false);

    // get a single sales item
    SalesItem get_sales_item(int id) noexcept(false);

    // Get receipt total
    double get_receipt_total(SaleItems items);

    // ================= USERS ======================================

    // create a new user, Returns id of inserted user.
    long long create_user(const User& user) noexcept(false);

    // Update user account
    void update_user(const User& user) noexcept(false);

    // get all users
    std::vector<User> get_users() noexcept(false);

    // get a single user
    User get_user(int id) noexcept(false);

    // get a single user by username
    User get_user_by_username(const std::string& username) noexcept(false);

    // delete a user
    void delete_user(int id) noexcept(false);

    // check if a user exists. Returns true if the username exists
    bool user_exists(const std::string& username) noexcept(false);

    // authenticate a user. Returns true if the username and password match
    bool authenticate_user(const std::string& username,
                           const std::string& password) noexcept(false);

    // =============== STOCK IN =====================================
    // create a new stock in item. Returns id of inserted item.
    long long create_stock_in(const StockIn& stockin, bool transaction = true) noexcept(false);

    // Insert multiple stock in items. Returns vector of inserted item ids.
    std::vector<long long> create_stock_in_items(const std::vector<StockIn>& items) noexcept(false);

    // get all stock in items
    std::vector<StockIn> get_stock_in_items() noexcept(false);

    // search for stock in items by invoice number, batch number, item name
    // Valid columns are: invoice_no, batch_no, item_id.
    // Returns a vector of stock in items that match the query
    std::vector<StockIn> search_stock_in_items(const std::string& column,
                                               const std::string& query) noexcept(false);

    // get a single stock in item
    StockIn get_stock_in_item(int id) noexcept(false);

    // get stock in items by item id
    std::vector<StockIn> get_stock_in_items_by_item_id(int item_id) noexcept(false);

    // Delete a stock in item.
    // This will also update the quantity of the inventory item
    // (reduce it by the quantity of the stock)
    void delete_stock_in_item(int id) noexcept(false);

    // =============== SALES REPORT =====================================
    // create a new sales report. Returns id of inserted item.
    std::vector<SalesReport> get_sales_report(const std::string& start_date,
                                              const std::string& end_date,
                                              const std::string& item_name = "",
                                              const std::string& item_brand = "") noexcept(false);
};
}  // namespace epharma
#endif /* EPHARMA_H */
