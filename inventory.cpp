#include "inventory.h"

#include <QtGlobal>

Inventory::Inventory(int capacity)
    : m_capacity(qMax(0, capacity)) {
}

int Inventory::usedCapacity() const {
    int total = 0;
    for (auto it = m_counts.cbegin(); it != m_counts.cend(); ++it) {
        total += it.value();
    }
    return total;
}

int Inventory::count(CropType type) const {
    return m_counts.value(static_cast<int>(type), 0);
}

bool Inventory::canAdd(CropType type, int quantity) const {
    return type != CropType::None && quantity > 0 &&
           quantity <= remainingCapacity();
}

bool Inventory::add(CropType type, int quantity) {
    if (!canAdd(type, quantity)) {
        return false;
    }
    m_counts[static_cast<int>(type)] += quantity;
    return true;
}

bool Inventory::remove(CropType type, int quantity) {
    if (type == CropType::None || quantity <= 0 || count(type) < quantity) {
        return false;
    }
    const int key = static_cast<int>(type);
    const int remaining = m_counts.value(key) - quantity;
    if (remaining == 0) {
        m_counts.remove(key);
    } else {
        m_counts[key] = remaining;
    }
    return true;
}

void Inventory::setCapacity(int capacity) {
    m_capacity = qMax(usedCapacity(), capacity);
}

bool Inventory::restore(int capacity, const QHash<int, int> &counts) {
    if (capacity < 0) {
        return false;
    }
    int used = 0;
    for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
        const CropType type = static_cast<CropType>(it.key());
        if (CropCatalog::definition(type).type == CropType::None ||
            it.value() < 0 || used > capacity - it.value()) {
            return false;
        }
        used += it.value();
    }
    m_capacity = capacity;
    m_counts = counts;
    return true;
}
