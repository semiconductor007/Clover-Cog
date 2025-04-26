#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "itembar.h"
#include "mapwidget.h"
#include "rpg.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);


private:
    ItemBar *itemBar;
    MapWidget *mapWidget;
    ImageMovementWidget *character;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override; // 添加事件过滤器
};

#endif // MAINWINDOW_H
