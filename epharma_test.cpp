#include "epharma.hpp"
#include <gtest/gtest.h>
#include <cstddef>
#include <exception>
#include <stdexcept>
#include <vector>

TEST(DateTimeTest, Conversion) {
    epharma::date_time now = std::chrono::system_clock::now();
    std::string now_str = epharma::dateTimeToString(now);  // discards milliseconds
    epharma::date_time now_back = epharma::dateTimeFromString(now_str);
    EXPECT_EQ(epharma::dateTimeToString(now), epharma::dateTimeToString(now_back));
}

TEST(DateTest, Conversion) {
    epharma::date today = std::chrono::system_clock::now();
    std::string today_str = epharma::dateToString(today);
    epharma::date today_back = epharma::dateFromString(today_str);
    EXPECT_EQ(epharma::dateToString(today), epharma::dateToString(today_back));
}

TEST(ExpiryDateTest, ValidDates) {
    EXPECT_TRUE(epharma::validate_expiry_date("2023-12-31"));
    EXPECT_TRUE(epharma::validate_expiry_date("2025-01-01"));
    // test a valid leap year
    EXPECT_TRUE(epharma::validate_expiry_date("2024-02-29"));
}

TEST(ExpiryDateTest, InvalidDates) {
    EXPECT_FALSE(epharma::validate_expiry_date("2021-02-29"));  // 2021 is not a leap year
    EXPECT_FALSE(epharma::validate_expiry_date("2023-13-01"));  // There is no 13th month
    EXPECT_FALSE(epharma::validate_expiry_date("2023-12-32"));  // There is no 32nd date
    EXPECT_FALSE(epharma::validate_expiry_date("abcd-ef-gh"));  // Not a date
}

TEST(COMPUTE_DAYS_TO_EXPIRY, ValidDates) {
    int daysToExpire =
        epharma::compute_days_to_expiry("2024-02-04", epharma::dateFromString("2024-02-04"));
    ASSERT_EQ(daysToExpire, 0);

    daysToExpire =
        epharma::compute_days_to_expiry("2024-02-10", epharma::dateFromString("2024-02-04"));
    ASSERT_EQ(daysToExpire, 6);  // expires in 3 days

    daysToExpire =
        epharma::compute_days_to_expiry("2024-02-01", epharma::dateFromString("2024-02-04"));
    ASSERT_EQ(daysToExpire, -3);  // expired 3 days ago

    daysToExpire =
        epharma::compute_days_to_expiry("2024-05-31", epharma::dateFromString("2024-06-30"));
    ASSERT_EQ(daysToExpire, -30);  // expired 30 days ago

    daysToExpire =
        epharma::compute_days_to_expiry("2023-12-31", epharma::dateFromString("2024-06-28"));
    ASSERT_EQ(daysToExpire, -180);  // expired 6 months ago.
}

TEST(EpharmaTest, TestInsertInventoryItems) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    std::vector<epharma::InventoryItem> items = {
        {1, "Paracetamol", "GSK", 100, 10.0, 15.0, "2022-12-31"},
        {2, "Amoxicillin", "GSK", 100, 10.0, 15.0, "2022-12-31"},
        {3, "Ibuprofen", "GSK", 100, 10.0, 15.0, "2022-12-31"},
        {4, "Ciprofloxacin", "GSK", 100, 10.0, 15.0, "2022-12-31"},
        {5, "Azithromycin", "GSK", 100, 10.0, 15.0, "2022-12-31"},
    };

    epharma.insert_inventory_items(items);

    auto result = epharma.get_inventory_items();
    ASSERT_EQ(result.size(), 5);
    ASSERT_EQ(result[0].id, 1);
    ASSERT_EQ(result[1].id, 2);
    ASSERT_EQ(result[2].id, 3);
    ASSERT_EQ(result[3].id, 4);
    ASSERT_EQ(result[4].id, 5);
    std::cout << result[0] << std::endl;
}

TEST(EpharmaTest, TestCreateInventoryItem) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    epharma.create_inventory_item(
        epharma::InventoryItem{6, "Doxycycline", "GSK", 100, 10.0, 15.0, "2022-12-31"});

    auto result = epharma.get_inventory_items();
    ASSERT_EQ(result.size(), 1);
    ASSERT_GT(result[0].id, 0);
    ASSERT_EQ(result[0].name, "Doxycycline");
}

TEST(EpharmaTest, TestUpdateInventoryItem) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    epharma.create_inventory_item(
        epharma::InventoryItem{6, "Doxycycline", "GSK", 100, 10.0, 15.0, "2022-12-31"});

    epharma.update_inventory_item(
        epharma::InventoryItem{6, "Doxycycline", "GSK", 300, 20.0, 15.0, "2022-12-31"});

    auto result = epharma.get_inventory_items();
    std::cout << result.size() << std::endl;

    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].id, 6);
    ASSERT_EQ(result[0].quantity, 300);
    ASSERT_EQ(result[0].cost_price, 20.0);
}

TEST(EpharmaTest, TestDeleteInventoryItem) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    epharma.create_inventory_item(
        epharma::InventoryItem{6, "Doxycycline", "GSK", 100, 10.0, 15.0, "2022-12-31"});

    epharma.delete_inventory_item(6);

    auto result = epharma.get_inventory_items();
    ASSERT_EQ(result.size(), 0);
}

TEST(EPharmaTest, UpdateBarcode) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");
    epharma.create_inventory_item(
        epharma::InventoryItem{1, "Doxycycline", "GSK", 100, 10.0, 15.0, "2022-12-31"});
    epharma.update_barcode(1, "new_barcode");

    epharma::InventoryItem item = epharma.get_inventory_item(1);
    ASSERT_EQ(item.barcode, "new_barcode");
}

TEST(EPharmaTest, GetInventoryItemByBarcode) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");
    epharma.create_inventory_item(
        epharma::InventoryItem{2, "Doxycycline", "GSK", 100, 10.0, 15.0, "2022-12-31"});

    epharma.update_barcode(2, "barcode2");
    epharma::InventoryItem item = epharma.get_inventory_item_by_barcode("barcode2");
    ASSERT_EQ(item.id, 2);
    ASSERT_EQ(item.name, "Doxycycline");
}

TEST(EpharmaTest, TestInsertSalesItems) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    epharma.create_inventory_item(
        epharma::InventoryItem{1, "Doxycycline-1", "GSK-1", 100, 10.0, 15.0, "2022-12-31"});
    epharma.create_inventory_item(
        epharma::InventoryItem{2, "Doxycycline-2", "GSK-2", 100, 10.0, 15.0, "2022-12-31"});

    std::vector<epharma::SalesItem> items = {
        {1, 1, "Paracetamol", 10, 10.0, 15.0},  {2, 2, "Amoxicillin", 10, 10.0, 15.0},
        {3, 1, "Ibuprofen", 10, 10.0, 15.0},    {4, 2, "Ciprofloxacin", 10, 10.0, 15.0},
        {5, 1, "Azithromycin", 10, 10.0, 15.0},
    };

    epharma.create_sales_items(items);

    auto result = epharma.get_sales_items();
    ASSERT_EQ(result.size(), 5);
    ASSERT_EQ(result[0].id, 1);
    ASSERT_EQ(result[1].id, 2);
    ASSERT_EQ(result[2].id, 3);
    ASSERT_EQ(result[3].id, 4);
    ASSERT_EQ(result[4].id, 5);
}

TEST(EpharmaTest, TestCreateSalesItem) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    epharma.create_inventory_item(
        epharma::InventoryItem{1, "Doxycycline-1", "GSK-1", 100, 10.0, 15.0, "2022-12-31"});

    epharma.create_sales_item(epharma::SalesItem{6, 1, "Doxycycline", 10, 10.0, 15.0});

    auto result = epharma.get_sales_items();
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].id, 6);
    ASSERT_EQ(result[0].item_id, 1);
    ASSERT_EQ(result[0].item_name, "Doxycycline");
}

TEST(EpharmaTest, TestTotalCostPrice) {
    epharma::SalesItem item{6, 6, "Doxycycline", 100, 10.0, 15.0};
    ASSERT_EQ(item.total_cost_price(), 1000.0);
}

TEST(EpharmaTest, TestTotalSellingPrice) {
    epharma::SalesItem item{6, 6, "Doxycycline", 100, 10.0, 15.0};
    ASSERT_EQ(item.total_selling_price(), 1500.0);
}

TEST(EpharmaTest, TestTotalProfit) {
    epharma::SalesItem item{6, 6, "Doxycycline", 100, 10.0, 15.0};
    ASSERT_EQ(item.total_profit(), 500.0);
}

TEST(EpharmaTest, TestReceiptTotal) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    std::vector<epharma::InventoryItem> inventory_items = {
        {1, "Paracetamol", "GSK", 100, 10.0, 15.0},  {2, "Amoxicillin", "GSK", 100, 10.0, 15.0},
        {3, "Ibuprofen", "GSK", 100, 10.0, 15.0},    {4, "Ciprofloxacin", "GSK", 100, 10.0, 15.0},
        {5, "Azithromycin", "GSK", 100, 10.0, 15.0},
    };

    epharma.insert_inventory_items(inventory_items);

    std::vector<epharma::SalesItem> items = {
        {1, 1, "Paracetamol", 5, 10.0, 500.0},  {2, 2, "Amoxicillin", 3, 10.0, 1000.0},
        {3, 3, "Ibuprofen", 2, 10.0, 15.0},     {4, 4, "Ciprofloxacin", 1, 10.0, 15.0},
        {5, 5, "Azithromycin", 8, 10.0, 450.0},
    };

    epharma.create_sales_items(items);

    double expectedReceiptTotal = epharma.get_receipt_total(items);
    ASSERT_EQ(expectedReceiptTotal, 9145);
}

TEST(EpharmaTest, CantMakeSalesIfOutOfStock) {
    std::vector<epharma::InventoryItem> inventory_items = {
        {1, "Paracetamol", "GSK", 100, 10.0, 15.0},
        {2, "Ibuprofen", "GSK", 100, 10.0, 15.0},
    };

    epharma::Epharma epharma;
    epharma.connect(":memory:");

    epharma.insert_inventory_items(inventory_items);
    std::vector<epharma::SalesItem> items = {
        {1, 1, "Paracetamol", 200, 10.0, 500.0},
        {2, 2, "Ibuprofen", 150, 10.0, 15.0},
    };

    // Should fail, we are selling more than what we have in stock.
    ASSERT_THROW(epharma.create_sales_items(items), std::runtime_error);
    try {
        epharma.create_sales_items(items);
    } catch (std::exception& e) {
        std::string error(e.what());
        size_t index = error.find("Insufficient quantity for item");
        ASSERT_NE(index, -1);
    }
}

// test search
TEST(EpharmaTest, TestSearchInventoryItems) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    std::vector<epharma::InventoryItem> items = {
        {1, "Paracetamol", "GSK", 100, 10.0, 15.0},  {2, "Amoxicillin", "GSK", 100, 10.0, 15.0},
        {3, "Ibuprofen", "GSK", 100, 10.0, 15.0},    {4, "Ciprofloxacin", "GSK", 100, 10.0, 15.0},
        {5, "Azithromycin", "GSK", 100, 10.0, 15.0},
    };

    epharma.insert_inventory_items(items);

    auto result = epharma.search_inventory_items("Paracetamol");
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].name, "Paracetamol");
}

TEST(EpharmaTest, TestSearchSalesItems) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    std::vector<epharma::InventoryItem> inventory_items = {
        {1, "Paracetamol", "GSK", 100, 10.0, 15.0},  {2, "Amoxicillin", "GSK", 100, 10.0, 15.0},
        {3, "Ibuprofen", "GSK", 100, 10.0, 15.0},    {4, "Ciprofloxacin", "GSK", 100, 10.0, 15.0},
        {5, "Azithromycin", "GSK", 100, 10.0, 15.0},
    };

    epharma.insert_inventory_items(inventory_items);

    std::vector<epharma::SalesItem> items = {
        {1, 1, "Paracetamol", 10, 10.0, 15.0},  {2, 2, "Amoxicillin", 10, 10.0, 15.0},
        {3, 3, "Ibuprofen", 10, 10.0, 15.0},    {4, 4, "Ciprofloxacin", 10, 10.0, 15.0},
        {5, 5, "Azithromycin", 10, 10.0, 15.0},
    };

    epharma.create_sales_items(items);

    auto result = epharma.search_sales_items("Paracetamol");
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].id, 1);
    ASSERT_EQ(result[0].item_name, "Paracetamol");

    // fuzzy search
    result = epharma.search_sales_items("cin");
    ASSERT_EQ(result.size(), 2);
}

// USER tests
TEST(EpharmaTest, TestCreateUser) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    epharma.create_user(epharma::User{1, "admin", "admin"});

    auto result = epharma.get_users();
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].id, 1);
    ASSERT_EQ(result[0].username, "admin");
}

TEST(EpharmaTest, TestUpdateUser) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    epharma.create_user(epharma::User{1, "admin", "admin"});

    epharma.update_user(epharma::User{1, "superadmin", "admin_password"});

    auto result = epharma.get_users();
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].id, 1);
    ASSERT_EQ(result[0].username, "superadmin");
    ASSERT_EQ(result[0].password, "admin_password");

    std::cout << result[0] << std::endl;
}

TEST(EpharmaTest, TestDeleteUser) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    epharma.create_user(epharma::User{1, "admin", "admin"});

    epharma.delete_user(1);

    auto result = epharma.get_users();
    ASSERT_EQ(result.size(), 0);
}

TEST(EpharmaTest, TestUserExists) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    epharma.create_user(epharma::User{1, "admin", "admin"});

    ASSERT_TRUE(epharma.user_exists("admin"));
    ASSERT_FALSE(epharma.user_exists("superadmin"));
}

TEST(EpharmaTest, TestAuthenticateUser) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    epharma.create_user(epharma::User{1, "admin", "admin"});

    ASSERT_TRUE(epharma.authenticate_user("admin", "admin"));
    ASSERT_FALSE(epharma.authenticate_user("admin", "superadmin"));
}

TEST(EpharmaTest, TestCreateStockInItem) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    epharma.create_inventory_item(
        epharma::InventoryItem{1, "Doxycycline", "GSK", 100, 10.0, 15.0, "2022-12-31"});

    epharma.create_stock_in(epharma::StockIn{1, 1, 100, "12345", "B1034", "2022-12-31"});

    auto result = epharma.get_inventory_items();
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].id, 1);
    ASSERT_EQ(result[0].quantity, 200);
}

// test search
TEST(EpharmaTest, TestSearchStockInItems) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    epharma.create_inventory_item(
        epharma::InventoryItem{1, "Doxycycline", "GSK", 100, 10.0, 15.0, "2022-12-31"});

    epharma.create_stock_in(epharma::StockIn{1, 1, 100, "12345", "B1034", "2022-12-31"});

    auto result = epharma.search_stock_in_items("invoice_no", "12345");
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].item_id, 1);
    ASSERT_EQ(result[0].quantity, 100);

    result = epharma.search_stock_in_items("batch_no", "B1034");
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].item_id, 1);
    ASSERT_EQ(result[0].quantity, 100);
}

TEST(EpharmaTest, TestGetStockInItemsByItemId) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    epharma.create_inventory_item(
        epharma::InventoryItem{1, "Doxycycline", "GSK", 100, 10.0, 15.0, "2022-12-31"});

    epharma.create_stock_in(epharma::StockIn{1, 1, 100, "12345", "B1034", "2022-12-31"});

    auto result = epharma.get_stock_in_items_by_item_id(1);
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].item_id, 1);
    ASSERT_EQ(result[0].quantity, 100);
}

// test multiple stock in items
TEST(EpharmaTest, TestCreateStockInItems) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    epharma.create_inventory_item(
        epharma::InventoryItem{1, "Doxycycline", "GSK", 100, 10.0, 15.0, "2022-12-31"});

    epharma.create_stock_in_items({
        epharma::StockIn{2, 1, 100, "12345", "B1034", "2022-12-31"},
        epharma::StockIn{3, 1, 200, "12346", "B1035", "2022-12-31"},
    });

    auto result = epharma.get_stock_in_items();
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0].item_id, 1);
    ASSERT_EQ(result[0].quantity, 100);

    ASSERT_EQ(result[1].item_id, 1);
    ASSERT_EQ(result[1].quantity, 200);

    auto item = epharma.get_inventory_item(result[0].item_id);
    ASSERT_EQ(item.quantity, 400);
}

TEST(EpharmaTest, TestGetStockInItems) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    epharma.create_inventory_item(
        epharma::InventoryItem{1, "Doxycycline", "GSK", 100, 10.0, 15.0, "2022-12-31"});

    epharma.create_stock_in_items({
        epharma::StockIn{2, 1, 100, "12345", "B1034", "2022-12-31"},
        epharma::StockIn{3, 1, 200, "12346", "B1035", "2022-12-31"},
    });

    auto result = epharma.get_stock_in_items();
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0].item_id, 1);
    ASSERT_EQ(result[0].quantity, 100);

    ASSERT_EQ(result[1].item_id, 1);
    ASSERT_EQ(result[1].quantity, 200);
}

// test delete stock in item
TEST(EpharmaTest, TestDeleteStockInItem) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    epharma.create_inventory_item(
        epharma::InventoryItem{1, "Doxycycline", "GSK", 200, 10.0, 15.0, "2022-12-31"});

    epharma.create_stock_in(epharma::StockIn{1, 1, 100, "12345", "B1034", "2022-12-31"});

    epharma.delete_stock_in_item(1);

    auto item = epharma.get_inventory_item(1);
    ASSERT_EQ(item.quantity, 200);  // no change in quantity
}

// test get_sales_report
TEST(EpharmaTest, TestGetSalesReport) {
    epharma::Epharma epharma;
    epharma.connect(":memory:");

    epharma.create_inventory_item(
        epharma::InventoryItem{1, "Doxycycline", "GSK", 200, 10.0, 15.0, "2022-12-31"});
    epharma.create_inventory_item(
        epharma::InventoryItem{2, "Paracetamol", "GSK", 200, 10.0, 15.0, "2022-12-31"});
    epharma.create_inventory_item(
        epharma::InventoryItem{3, "Amoxicillin", "GSK", 200, 10.0, 15.0, "2022-12-31"});

    epharma.create_sales_item(epharma::SalesItem{3, 3, "Amoxicillin", 100, 10.0, 15.0});
    epharma.create_sales_item(epharma::SalesItem{1, 1, "Doxycycline", 100, 10.0, 15.0});
    epharma.create_sales_item(epharma::SalesItem{2, 1, "Doxycycline", 100, 10.0, 15.0});

    auto start_date = epharma::dateToString(std::chrono::system_clock::now());
    auto end_date = epharma::dateToString(std::chrono::system_clock::now());

    std::vector<epharma::SalesReport> result = epharma.get_sales_report(start_date, end_date);
    ASSERT_EQ(result.size(), 2);

    ASSERT_EQ(result[0].item_name, "Amoxicillin");  // DESC order
    ASSERT_EQ(result[0].total_quantity_sold, 100);
    ASSERT_EQ(result[0].total_cost_price, 1000.0);
    ASSERT_EQ(result[0].total_selling_price, 1500.0);
    ASSERT_EQ(result[0].total_profit, 500.0);

    ASSERT_EQ(result[1].item_name, "Doxycycline");
    ASSERT_EQ(result[1].total_quantity_sold, 200);
    ASSERT_EQ(result[1].total_cost_price, 2000.0);
    ASSERT_EQ(result[1].total_selling_price, 3000.0);
    ASSERT_EQ(result[1].total_profit, 1000.0);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
