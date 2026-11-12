#include "daynightenvironment.h"
#include "seasonvisuals.h"

#include <array>

QColor DayNightEnvironment::tintAt(qint64 gameMinutes, bool outdoors, Season season) {
    if (!outdoors) return QColor(0, 0, 0, 0);
    struct Keyframe { int minute; QColor tint; };
    // QPainter source-over values, deliberately distinct from Stardew's lightmap
    // blend factors. These are visual trial values, not confirmed game rules.
    const auto settings = SeasonVisuals::profile(season);
    QColor transparent = settings.twilightTint;
    transparent.setAlpha(0);
    const std::array<Keyframe, 8> profile{{
        {0, settings.nightTint},
        {settings.dawnStart, settings.nightTint},
        {settings.dawnMiddle, settings.twilightTint},
        {settings.dawnEnd, transparent},
        {settings.duskStart, transparent},
        {settings.duskMiddle, settings.twilightTint},
        {settings.duskEnd, settings.nightTint},
        {1440, settings.nightTint}
    }};
    const int minute = static_cast<int>(qMax<qint64>(0, gameMinutes) % 1440);
    for (size_t i = 1; i < profile.size(); ++i) {
        const auto &left = profile[i - 1];
        const auto &right = profile[i];
        if (minute > right.minute) continue;
        const double fraction = double(minute - left.minute) / (right.minute - left.minute);
        const auto mix = [fraction](int a, int b) { return qRound(a + (b - a) * fraction); };
        return QColor(mix(left.tint.red(), right.tint.red()),
                      mix(left.tint.green(), right.tint.green()),
                      mix(left.tint.blue(), right.tint.blue()),
                      mix(left.tint.alpha(), right.tint.alpha()));
    }
    return profile.back().tint;
}
