#include "ui/scenetransition.h"

#include <QtTest>
#include <QTimer>

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

QTEST_MAIN(SceneTransitionTests)
#include "scenetransition_tests.moc"
