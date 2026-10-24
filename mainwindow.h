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
#include "economyservice.h"
#include "gameclock.h"
#include "itembar.h"
#include "mapwidget.h"
#include "rpg.h"
#include "savemanager.h"



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
    MainWindow(QWidget *parent = nullptr);
    int currentGameHour() const { return m_gameClock.hour(); }
    int currentGameDay() const { return m_gameClock.day(); }
    int currentGameMinute() const { return m_gameClock.minute(); }


private slots:
    void updateTime();
    void sleepUntilMorning();

protected:
    void mousePressEvent(QMouseEvent *event) override;  // 声明鼠标按下事件
    void mouseReleaseEvent(QMouseEvent *event) override; // 声明鼠标释放事件
    void closeEvent(QCloseEvent *event) override;

private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    GameClock m_gameClock;
    int m_lastProcessedDay = 0;
    bool m_saveEnabled = true;
    EconomyService m_economy;
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
    bool saveGame(bool showError = true);
    bool loadGame();
    DayAdvanceResult processDayTransitions();

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
