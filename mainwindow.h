// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "itembar.h"
#include "mapwidget.h"
#include "rpg.h"
#include <QSoundEffect>
#include <QMediaPlayer>
#include <QAudioOutput>



class ClockBgWidget : public QWidget {
public:
    explicit ClockBgWidget(QWidget *parent = nullptr) : QWidget(parent) {}
    void setBackground(const QPixmap &bg) { clockBg = bg; update(); }
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QPixmap clockBg;
    QPixmap m_cropNumImage;

};


// 在MainWindow类定义中添加
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    int currentGameHour() const { return m_gameTimer.elapsed() / 60000 % 24; }
    int currentGameDay() const { return m_gameTimer.elapsed() / 86400000; }
    int currentGameMinute() const {  return (m_gameTimer.elapsed() / 1000) % 60; };


private slots:
    void updateTime();

private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    QElapsedTimer m_gameTimer; // 移到MainWindow类中
    MapWidget* mapWidget;  // 添加mapWidget成员声明
    ImageMovementWidget* character;
    ItemBar* itemBar;

    QWidget *overlay; // 添加覆盖层控件声明
    QLabel *m_wheatCountLabel;
    int m_wheatCount = 0;
    void updateWheatCountDisplay();

    ClockBgWidget *clockBgWidget;
    QPixmap background;

    QLabel *timeLabel;    // 显示时间的标签
    QFont digitalFont;    // 自定义字体对象
    QLabel *dayLabel;

    bool loadCustomFont(); // 加载字体
    void resizeEvent(QResizeEvent *event) override;
    void loadBackground();   // 加载背景图

    QMediaPlayer *m_player;
    QAudioOutput *m_audioOutput;

};

#endif // MAINWINDOW_H
