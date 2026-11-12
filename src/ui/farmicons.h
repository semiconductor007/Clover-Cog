#ifndef FARMICONS_H
#define FARMICONS_H

#include "farm/cropdefinition.h"
#include <QPixmap>

namespace FarmIcons {
inline QPixmap crop(const CropIcon &icon) {
    return QPixmap(icon.resourcePath).copy(icon.sourceRect);
}

inline QPixmap fertilizer() {
    return QPixmap(QStringLiteral(":/farm-ui/soil-items.png")).copy(0, 0, 16, 16);
}
} // namespace FarmIcons

#endif
