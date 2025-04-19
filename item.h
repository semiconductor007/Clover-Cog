// item.h
#ifndef ITEM_H
#define ITEM_H

#include <QIcon>
#include <QString>

class Item {
public:
    enum ItemType {
        Weapon,      // 武器
        Tool,        // 工具（斧头、镐等）
        Consumable,  // 消耗品
        Material,    // 材料
        Empty        // 空槽位
    };

    Item(int id = -1,
         ItemType type = Empty,
         const QString& name = "",
         const QIcon& icon = QIcon(),
         int maxStack = 1,
         bool equipable = false)
        : m_id(id),
        m_type(type),
        m_name(name),
        m_icon(icon),
        m_maxStack(maxStack),
        m_equipable(equipable),
        m_quantity(1),
        m_equipped(false) {}

    // Getters
    int id() const { return m_id; }
    ItemType type() const { return m_type; }
    QString name() const { return m_name; }
    QIcon icon() const { return m_icon; }
    int maxStack() const { return m_maxStack; }
    bool isEquipable() const { return m_equipable; }
    int quantity() const { return m_quantity; }
    bool isEquipped() const { return m_equipped; }

    // Setters
    void setQuantity(int q) { m_quantity = q; }
    void setEquipped(bool e) { m_equipped = e; }

private:
    int m_id;
    ItemType m_type;
    QString m_name;
    QIcon m_icon;
    int m_maxStack;
    bool m_equipable;
    int m_quantity;
    bool m_equipped;
};

#endif // ITEM_H
