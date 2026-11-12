#include "calendarpresentation.h"

#include <QFontDatabase>
#include <QRawFont>

CalendarFont CalendarPresentation::chooseFont() {
    const QString glyphs = QStringLiteral("第年春夏秋冬季天进入景观通用同日休息早晨至四专属尚未配置保留当前地图与农田状态：");
    const QStringList families = QFontDatabase::families();
    for (const QString &family : {QStringLiteral("Microsoft YaHei UI"),
                                 QStringLiteral("Microsoft YaHei"),
                                 QStringLiteral("Noto Sans CJK SC"),
                                 QStringLiteral("PingFang SC")}) {
        if (!families.contains(family)) continue;
        const QFont font(family, 9);
        const QRawFont raw = QRawFont::fromFont(font);
        bool supported = raw.isValid();
        for (QChar c : glyphs) supported &= raw.supportsCharacter(c.unicode());
        if (supported) return {font, true};
    }
    return {QFontDatabase::systemFont(QFontDatabase::GeneralFont), false};
}

QString CalendarPresentation::seasonName(Season season, bool chinese) {
    if (!chinese) return GameCalendar::seasonKey(season).toUpper();
    switch (season) {
    case Season::Spring: return QStringLiteral("春季");
    case Season::Summer: return QStringLiteral("夏季");
    case Season::Autumn: return QStringLiteral("秋季");
    case Season::Winter: return QStringLiteral("冬季");
    }
    return QStringLiteral("?");
}

QString CalendarPresentation::dateText(const CalendarDate &date, bool chinese, bool twoLines) {
    const QString separator = twoLines ? QStringLiteral("\n") : QStringLiteral(" ");
    if (chinese)
        return QStringLiteral("第%1年 %2%3第%4天").arg(date.year)
            .arg(seasonName(date.season, true), separator).arg(date.dayOfSeason);
    return QStringLiteral("Y%1 %2%3DAY %4").arg(date.year)
        .arg(seasonName(date.season, false), separator).arg(date.dayOfSeason);
}
