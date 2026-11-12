#ifndef SEASONVISUALS_H
#define SEASONVISUALS_H

#include "gamecalendar.h"
#include "mapdata.h"
#include <QColor>
#include <QMap>
#include <QStringList>

// Presentation configuration. Prototype palette variants, not authored snow maps.
struct SeasonVisualProfile {
    Season season = Season::Spring;
    int leafHue = -1;
    double saturationScale = 1;
    double valueScale = 1;
    double valueLift = 0;
    int dawnStart = 300, dawnMiddle = 360, dawnEnd = 420;
    int duskStart = 1020, duskMiddle = 1140, duskEnd = 1260;
    QColor nightTint{24, 34, 72, 135};
    QColor twilightTint{158, 85, 55, 38};
    // Keys are exact TSX filenames. Overrides must retain grid/alpha layout.
    QMap<QString, QString> textureOverrides;
    QString cacheKey() const;
    bool isValid() const;
};

namespace SeasonVisuals {
SeasonVisualProfile profile(Season season);
QStringList vegetationAtlases();
bool build(const QVector<Tileset> &base, const SeasonVisualProfile &profile,
           QVector<Tileset> *result, QString *error = nullptr);
}

#endif
