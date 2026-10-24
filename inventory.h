#ifndef INVENTORY_H
#define INVENTORY_H

#include "cropdefinition.h"

#include <QHash>

class Inventory {
public:
    explicit Inventory(int capacity = 12);

    int capacity() const { return m_capacity; }
    int usedCapacity() const;
    int remainingCapacity() const { return m_capacity - usedCapacity(); }
    int count(CropType type) const;

    bool canAdd(CropType type, int quantity) const;
    bool add(CropType type, int quantity);
    bool remove(CropType type, int quantity);
    void setCapacity(int capacity);
    const QHash<int, int> &counts() const { return m_counts; }
    bool restore(int capacity, const QHash<int, int> &counts);

private:
    int m_capacity;
    QHash<int, int> m_counts;
};

#endif // INVENTORY_H
