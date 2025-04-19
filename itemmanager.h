// itemmanager.h
#include <QJsonDocument>
#include <QFile>
#include "Item.h"

class ItemManager {
public:
    static QMap<int, Item> loadItems(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "无法打开物品文件:" << path;
            return {};
        }

        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonArray itemsArray = doc.array();

        QMap<int, Item> items;
        for (const QJsonValue& val : itemsArray) {
            QJsonObject obj = val.toObject();
            Item item(
                obj["id"].toInt(),
                static_cast<Item::ItemType>(obj["type"].toInt()),
                obj["name"].toString(),
                QIcon(obj["iconPath"].toString()),
                obj["maxStack"].toInt(1),
                obj["equipable"].toBool()
                );
            items.insert(item.id(), item);
        }
        return items;
    }
};
