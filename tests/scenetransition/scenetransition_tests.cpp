#include "ui/scenetransition.h"
#include "farm/seasoncalendar.h"

#include <QtTest>
#include <QTimer>
#include <limits>

namespace {

void fillWidget(QWidget &widget, const QColor &color) {
    QPalette palette = widget.palette();
    palette.setColor(QPalette::Window, color);
    widget.setPalette(palette);
    widget.setAutoFillBackground(true);
}

QColor pixelAt(QWidget &canvas, const QPoint &position) {
    const QImage image = canvas.grab().toImage();
    return image.pixelColor(position * image.devicePixelRatio());
}

} // namespace

class SceneTransitionTests : public QObject {
    Q_OBJECT
private slots:
    void swapsCommittedSceneBehindBlack();
    void runsOnEventLoopAndCanPlayAgain();
    void lightingFollowsClockAndLeavesHudInteractive();
    void springKeepsOriginalColours_data();
    void springKeepsOriginalColours();
    void seasonalDuskStartsAtPlannedTime_data();
    void seasonalDuskStartsAtPlannedTime();
    void summerEveningIsBrighterAndWinterMorningIsReadable();
    void lightIsContinuousAcrossDaysAndSeasons();
    void lightUsesTotalElapsedMinutes();
};

void SceneTransitionTests::swapsCommittedSceneBehindBlack() {
    QWidget canvas;
    canvas.resize(80, 60);
    fillWidget(canvas, Qt::red);
    SceneTransition mask(&canvas);
    canvas.show();
    QSignalSpy black(&mask, &SceneTransition::blackReached);
    QSignalSpy finished(&mask, &SceneTransition::finished);
    const QPoint sample(40, 30);

    QColor atSwap;
    connect(&mask, &SceneTransition::blackReached, &canvas, [&]() {
        atSwap = pixelAt(canvas, sample);
        QVERIFY(!mask.start());
    });
    QVERIFY(mask.start());
    fillWidget(canvas, Qt::green);
    // A tick or harvest may have already changed the real world behind the old frame.
    QCOMPARE(pixelAt(canvas, sample), QColor(Qt::red));
    auto *animation = mask.findChild<QSequentialAnimationGroup *>();
    QVERIFY(animation);
    QCOMPARE(animation->duration(), 900);
    animation->setCurrentTime(175);
    const QColor duringFadeOut = pixelAt(canvas, sample);
    QVERIFY(duringFadeOut.red() > 120 && duringFadeOut.red() < 136);
    QCOMPARE(duringFadeOut.green(), 0);

    animation->setCurrentTime(349);
    QCOMPARE(black.count(), 0);
    animation->setCurrentTime(350);
    QCOMPARE(black.count(), 1);
    QCOMPARE(atSwap, QColor(Qt::black));
    animation->setCurrentTime(449);
    QCOMPARE(pixelAt(canvas, sample), QColor(Qt::black));
    animation->setCurrentTime(675);
    const QColor duringReveal = pixelAt(canvas, sample);
    QCOMPARE(duringReveal.red(), 0);
    QVERIFY(duringReveal.green() > 120 && duringReveal.green() < 136);
    animation->setCurrentTime(900);
    QCOMPARE(black.count(), 1);
    QCOMPARE(finished.count(), 1);
    QVERIFY(!mask.isActive());
    QVERIFY(mask.isHidden());
    QCOMPARE(pixelAt(canvas, sample), QColor(Qt::green));
}

void SceneTransitionTests::runsOnEventLoopAndCanPlayAgain() {
    QWidget canvas;
    SceneTransition mask(&canvas);
    canvas.show();
    QSignalSpy black(&mask, &SceneTransition::blackReached);
    QSignalSpy finished(&mask, &SceneTransition::finished);
    for (int play = 1; play <= 2; ++play) {
        QVERIFY(mask.start());
        QVERIFY(!mask.start());
        QVERIFY(mask.isActive());
        bool delivered = false;
        QTimer::singleShot(0, &mask, [&]() { delivered = mask.isActive(); });
        QVERIFY(finished.wait(4000));
        QVERIFY(delivered);
        QCOMPARE(black.count(), play);
        QCOMPARE(finished.count(), play);
        QVERIFY(!mask.isActive());
    }
}

void SceneTransitionTests::lightingFollowsClockAndLeavesHudInteractive() {
    QWidget canvas;
    canvas.resize(80, 60);
    QWidget actor(&canvas);
    actor.setGeometry(1, 1, 24, 24);
    fillWidget(actor, Qt::white);
    DayNightOverlay light(&canvas);
    QWidget hud(&canvas);
    hud.setGeometry(40, 1, 24, 24);
    fillWidget(hud, Qt::white);
    canvas.show();
    canvas.resize(92, 62);
    QCOMPARE(light.geometry(), canvas.rect());
    QCOMPARE(canvas.childAt(10, 10), &actor);
    QCOMPARE(canvas.childAt(50, 10), &hud);

    const QPoint actorPixel(10, 10);
    light.setGameMinutes(12 * 60);
    QCOMPARE(pixelAt(canvas, actorPixel), QColor(Qt::white));
    light.setGameMinutes(23 * 60 + 59);
    const QColor night = pixelAt(canvas, actorPixel);
    QVERIFY(night.blue() > night.red());
    QVERIFY(night.red() > 126);
    light.setGameMinutes(24 * 60);
    QCOMPARE(pixelAt(canvas, actorPixel), night);
    QCOMPARE(pixelAt(canvas, QPoint(50, 10)), QColor(Qt::white));

    light.setGameMinutes(30 * 60);
    const QColor dawn = pixelAt(canvas, actorPixel);
    QVERIFY(dawn.red() > dawn.blue());
    QVERIFY(dawn.red() > night.red());
    QCOMPARE(pixelAt(canvas, QPoint(50, 10)), QColor(Qt::white));
}

void SceneTransitionTests::springKeepsOriginalColours_data() {
    QTest::addColumn<int>("minute");
    QTest::addColumn<QColor>("expected");
    QTest::newRow("night") << 0 << QColor(24, 36, 78, 124);
    QTest::newRow("wake-up") << 6 * 60 << QColor(250, 203, 150, 48);
    QTest::newRow("noon") << 12 * 60 << QColor(250, 203, 150, 0);
    QTest::newRow("sunset") << 18 * 60 + 30 << QColor(223, 139, 107, 58);
    QTest::newRow("twilight") << 20 * 60 << QColor(79, 69, 112, 94);
}

void SceneTransitionTests::springKeepsOriginalColours() {
    QFETCH(int, minute);
    QFETCH(QColor, expected);
    QCOMPARE(DayNightOverlay::tintForMinutes(minute), expected);
}

void SceneTransitionTests::seasonalDuskStartsAtPlannedTime_data() {
    QTest::addColumn<int>("seasonIndex");
    QTest::addColumn<int>("dusk");
    QTest::newRow("spring") << 0 << 16 * 60 + 30;
    QTest::newRow("summer") << 1 << 18 * 60;
    QTest::newRow("autumn") << 2 << 16 * 60;
    QTest::newRow("winter") << 3 << 15 * 60;
}

void SceneTransitionTests::seasonalDuskStartsAtPlannedTime() {
    QFETCH(int, seasonIndex);
    QFETCH(int, dusk);
    for (int dayInSeason : {0, SeasonCalendar::DaysPerSeason - 1}) {
        const qint64 day = seasonIndex * SeasonCalendar::DaysPerSeason + dayInSeason;
        const qint64 elapsed = day * 24 * 60;
        QCOMPARE(DayNightOverlay::tintForMinutes(elapsed + dusk - 1).alpha(), 0);
        QCOMPARE(DayNightOverlay::tintForMinutes(elapsed + dusk).alpha(), 0);
        QVERIFY(DayNightOverlay::tintForMinutes(elapsed + dusk + 30).alpha() > 0);
        int previousAlpha = 0;
        for (int minute = dusk; minute < 24 * 60; ++minute) {
            const int alpha = DayNightOverlay::tintForMinutes(elapsed + minute).alpha();
            QVERIFY(alpha >= previousAlpha);
            // Even the darkest night preserves over half the scene contrast.
            QVERIFY(alpha <= 124);
            previousAlpha = alpha;
        }
    }
}

void SceneTransitionTests::summerEveningIsBrighterAndWinterMorningIsReadable() {
    QWidget canvas;
    canvas.resize(100, 40);
    fillWidget(canvas, Qt::black);
    QWidget whiteTerrain(&canvas);
    whiteTerrain.setGeometry(0, 0, 30, 40);
    fillWidget(whiteTerrain, Qt::white);
    DayNightOverlay light(&canvas);
    QWidget hud(&canvas);
    hud.setGeometry(70, 0, 30, 40);
    fillWidget(hud, Qt::white);
    canvas.show();

    const qint64 summer = qint64(SeasonCalendar::DaysPerSeason) * 24 * 60;
    const qint64 winter = 3 * summer;
    const auto luma = [](QColor c) {
        return (2126 * c.red() + 7152 * c.green() + 722 * c.blue()) / 10000.0;
    };
    light.setGameMinutes(summer + 18 * 60);
    const QColor summerEvening = pixelAt(canvas, QPoint(10, 10));
    QCOMPARE(summerEvening, QColor(Qt::white));
    light.setGameMinutes(winter + 18 * 60);
    const QColor winterEvening = pixelAt(canvas, QPoint(10, 10));
    QVERIFY(luma(summerEvening) > luma(winterEvening) + 30);
    QCOMPARE(pixelAt(canvas, QPoint(80, 10)), QColor(Qt::white));
    QCOMPARE(canvas.childAt(80, 10), &hud);

    light.setGameMinutes(winter + 6 * 60);
    const QColor morningWhite = pixelAt(canvas, QPoint(10, 10));
    const QColor morningBlack = pixelAt(canvas, QPoint(50, 10));
    // A visible road/terrain edge retains at least 80% of its source contrast
    // at the winter wake-up time, and the HUD receives no scene tint.
    QVERIFY(luma(morningWhite) - luma(morningBlack) >= 0.80 * 255);
    QCOMPARE(pixelAt(canvas, QPoint(80, 10)), QColor(Qt::white));
    qInfo() << "18:00 rendered white terrain: summer" << summerEvening
            << "luma" << luma(summerEvening) << "winter" << winterEvening
            << "luma" << luma(winterEvening);
    qInfo() << "Winter 06:00 rendered edge contrast:"
            << luma(morningWhite) - luma(morningBlack) << "of 255";
}

void SceneTransitionTests::lightIsContinuousAcrossDaysAndSeasons() {
    const int daysPerYear = SeasonCalendar::DaysPerSeason * SeasonCalendar::SeasonsPerYear;
    for (int day = 1; day <= 2 * daysPerYear; ++day) {
        const qint64 midnight = qint64(day) * 24 * 60;
        const QColor before = DayNightOverlay::tintForMinutes(midnight - 1);
        QCOMPARE(DayNightOverlay::tintForMinutes(midnight), before);
        QCOMPARE(DayNightOverlay::tintForMinutes(midnight + 1), before);
    }
    for (int season = 0; season < SeasonCalendar::SeasonsPerYear; ++season) {
        const qint64 start = qint64(season) * SeasonCalendar::DaysPerSeason * 24 * 60;
        QColor previous = DayNightOverlay::tintForMinutes(start);
        for (int minute = 1; minute < 24 * 60; ++minute) {
            const QColor current = DayNightOverlay::tintForMinutes(start + minute);
            // No abrupt change at a dawn/dusk keyframe (one game minute).
            QVERIFY(qAbs(current.red() - previous.red()) <= 4);
            QVERIFY(qAbs(current.green() - previous.green()) <= 4);
            QVERIFY(qAbs(current.blue() - previous.blue()) <= 4);
            QVERIFY(qAbs(current.alpha() - previous.alpha()) <= 2);
            previous = current;
        }
    }
}

void SceneTransitionTests::lightUsesTotalElapsedMinutes() {
    const qint64 year = qint64(SeasonCalendar::DaysPerSeason)
        * SeasonCalendar::SeasonsPerYear * 24 * 60;
    // Loaded farms and large time jumps must select the same season without
    // narrowing elapsed minutes or keeping a separate, stale season counter.
    const qint64 distantYear = 100000 * year;
    QVERIFY(distantYear > std::numeric_limits<int>::max());
    for (int season = 0; season < SeasonCalendar::SeasonsPerYear; ++season) {
        const qint64 firstDay = qint64(season) * SeasonCalendar::DaysPerSeason * 24 * 60;
        for (int minute : {6 * 60, 12 * 60, 18 * 60, 23 * 60 + 59}) {
            QCOMPARE(DayNightOverlay::tintForMinutes(distantYear + firstDay + minute),
                     DayNightOverlay::tintForMinutes(firstDay + minute));
        }
    }
    QCOMPARE(DayNightOverlay::tintForMinutes(std::numeric_limits<qint64>::min()),
             DayNightOverlay::tintForMinutes(0));
}

QTEST_MAIN(SceneTransitionTests)
#include "scenetransition_tests.moc"
