#include "models.hpp"
#include <QJsonDocument>

QJsonObject Product::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["generic_name"] = genericName;
    obj["brand_name"] = brandName;
    obj["quantity"] = quantity;
    obj["cost_price"] = costPrice;
    obj["selling_price"] = sellingPrice;
    obj["barcode"] = barcode;
    QJsonArray dates;
    for (const auto& d : expiryDates) {
        dates.append(d.toString(Qt::ISODate));
    }
    obj["expiry_dates"] = dates;
    return obj;
}

Product Product::fromJson(const QJsonObject& obj) {
    Product p;
    p.id = obj["id"].toInt();
    p.genericName = obj["generic_name"].toString();
    p.brandName = obj["brand_name"].toString();
    p.quantity = obj["quantity"].toInt();
    p.costPrice = obj["cost_price"].toDouble();
    p.sellingPrice = obj["selling_price"].toDouble();
    p.barcode = obj["barcode"].toString();
    QJsonArray dates = obj["expiry_dates"].toArray();
    for (const auto& d : dates) {
        p.expiryDates.append(QDate::fromString(d.toString(), Qt::ISODate));
    }
    return p;
}

QJsonObject TransactionItem::toJson() const {
    QJsonObject obj;
    obj["id"] = productId;
    obj["generic_name"] = genericName;
    obj["brand_name"] = brandName;
    obj["quantity"] = quantity;
    obj["selling_price"] = sellingPrice;
    obj["cost_price"] = costPrice;
    obj["barcode"] = barcode;
    return obj;
}

TransactionItem TransactionItem::fromJson(const QJsonObject& obj) {
    TransactionItem item;
    item.productId = obj["id"].toInt();
    item.genericName = obj["generic_name"].toString();
    item.brandName = obj["brand_name"].toString();
    item.quantity = obj["quantity"].toInt();
    item.sellingPrice = obj["selling_price"].toDouble();
    item.costPrice = obj["cost_price"].toDouble();
    item.barcode = obj["barcode"].toString();
    return item;
}
