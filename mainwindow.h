// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAudioOutput>
#include <QCursor>
#include <QFont>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QMediaPlayer>
#include <QPixmap>
#include <QRect>
#include <QPointer>
#include <QDialog>
#include "economyservice.h"
#include "inventorypanel.h"
#include "gameclock.h"
#include "itembar.h"
#include "mapwidget.h"
#include "rpg.h"
#include "savemanager.h"
#include "sleepoverlay.h"
#include "daynightoverlay.h"
#include "calendarpresentation.h"
#include "sleepsoundcue.h"



class ClockBgWidget : public QWidget {
public:
    explicit ClockBgWidget(QWidget *parent = nullptr) : QWidget(parent) {}
    void setBackground(const QPixmap &bg) { clockBg = bg; update(); }
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QPixmap clockBg;

};


// 在MainWindow类定义中添加
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr, const QString &savePath = QString(),
               CalendarRules newGameCalendar = {});
    int currentGameHour() const { return m_gameClock.hour(); }
    int currentGameDay() const { return m_gameClock.day(); }
    int currentGameMinute() const { return m_gameClock.minute(); }
    void setEffectsVolume(float volume) { m_sleepSound->setVolume(volume); }
    void setEffectsMuted(bool muted) { m_sleepSound->setMuted(muted); }


signals:
    // Notification after a coherent day settlement; presentation only.
    void seasonChanged(qint64 year, int season);

private slots:
    void updateTime();
    void sleepUntilMorning();

protected:
    void mousePressEvent(QMouseEvent *event) override;  // 声明鼠标按下事件
    void mouseReleaseEvent(QMouseEvent *event) override; // 声明鼠标释放事件
    void closeEvent(QCloseEvent *event) override;

private:
    friend class SleepFlowTests;
    bool eventFilter(QObject *watched, QEvent *event) override;
    GameClock m_gameClock;
    CalendarRules m_calendarRules;
    CalendarDate m_displayedDate;
    bool m_hasDisplayedDate = false;
    CalendarFont m_calendarFont;
    void refreshCalendar();
    int m_lastProcessedDay = 0;
    bool m_saveEnabled = true;
    EconomyService m_economy;
    QString m_savePath;
    bool m_ready = false;
    bool m_processingDays = false;
    bool m_dayAdvanceFailed = false;
    bool m_sleepBusy = false;
    bool m_passedOut = false;
    bool m_enforcingPassOut = false;
    qint64 m_lastObservedGameMinutes = 360;
    bool enforcePassOut();
    void startSleep(bool passedOut);
    std::optional<QPoint> bedWakePosition() const;
    QPointer<QDialog> m_activeShop;
    bool m_sleepConfirming = false;
    bool m_confirmationWasPaused = false;
    void requestSleep();
    void confirmSleep();
    void cancelSleepConfirmation();
    SleepSoundCue *m_sleepSound = nullptr;
    QLabel *m_bedHintLabel = nullptr;
    bool m_sleepSaved = false;
    bool m_sleepSettled = false;
    qint64 m_sleepStartMinutes = 0;
    qint64 m_sleepTarget = 0;
    int m_sleepOldDay = 0;
    SleepOverlay *m_sleepOverlay = nullptr;
    DayNightOverlay *m_dayNightOverlay = nullptr;
    void completeSleep();
    const QRect m_marketArea{19, 11, 2, 2};
    const QRect m_shopArea{22, 11, 2, 2};
    const QRect m_bedArea{16, 11, 2, 2};
    QWidget *m_gameCanvas;
    QGraphicsScene *m_gameScene;
    QGraphicsView *m_gameView;
    MapWidget* mapWidget;  // 添加mapWidget成员声明
    ImageMovementWidget* character;
    ItemBar* itemBar;

    QWidget *overlay; // 添加覆盖层控件声明
    QLabel *m_coinsLabel = nullptr;
    QLabel *m_inventoryLabel = nullptr;
    QLabel *m_cropsLabel = nullptr;
    QLabel *m_seedsLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    void updateHud();
    void showStatus(const QString &message);
    void openSeedShop();
    void toggleInventory();
    InventoryPanel *m_inventoryPanel = nullptr;
    bool saveGame(bool showError = true);
    bool loadGame();
    DayAdvanceResult processDayTransitions(bool saveAfter = true);

    ClockBgWidget *clockBgWidget;
    QPixmap background;

    QLabel *timeLabel;    // 显示时间的标签
    QFont digitalFont;    // 自定义字体对象
    QLabel *dayLabel;

    void updateGameScale();
    bool loadCustomFont(); // 加载字体
    void resizeEvent(QResizeEvent *event) override;
    void loadBackground();   // 加载背景图

    QMediaPlayer *m_player;
    QAudioOutput *m_audioOutput;
    QCursor customCursor;
    QCursor clickCursor;

};

#endif // MAINWINDOW_H
