#include "mainwindow.h"
#include <QApplication>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // ==================== 窗口基础设置 ====================

    setFixedSize(520, 320);

    // 地图控件尺寸与主窗口一致
    mapWidget = new MapWidget(this);
    mapWidget->setFixedSize(520, 320); // 强制固定为视窗大小
    mapWidget->loadMap(":/map/map6.tmj");
    // ==================== 角色控件初始化 ====================
    character = new ImageMovementWidget(mapWidget);
    // 初始位置设置为地图中心（根据地图尺寸动态计算）
    QPoint initialPos(
        mapWidget->tileWidth() * mapWidget->mapWidth() / 2,
        mapWidget->tileHeight() * mapWidget->mapHeight() / 2
        );
    character->setInitialPosition(initialPos);

    // 初始化物品栏（父控件为 mapWidget）
    itemBar = new ItemBar(mapWidget);
    itemBar->raise(); // 显示在最上层

    // 设置物品栏位置和尺寸（221x54）
    const int itemBarWidth = 221;
    const int itemBarHeight = 54;
    itemBar->setGeometry(
        (mapWidget->width() - itemBarWidth) / 2, // 水平居中
        mapWidget->height() - itemBarHeight - 10, // 距离底部 10 像素
        itemBarWidth,
        itemBarHeight
        );

    // ==================== 信号连接 ====================
    // 物品栏选中物品时更新角色状态
    connect(itemBar, &ItemBar::itemSelected, [=](int index, const QString &item){
        qDebug() << "使用物品:" << index << item;
        // 这里可以添加物品使用逻辑
    });

    // 物品装备状态变化时更新角色属性
    connect(itemBar, &ItemBar::itemEquipped, [=](int slot, bool equipped){
        qDebug() << "装备状态变化 - 槽位:" << slot << "状态:" << equipped;
    });

     // 为窗口安装事件过滤器
    this->installEventFilter(this);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::Wheel) {
        // 将滚轮事件转发给 ItemBar
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
        itemBar->wheelEvent(wheelEvent); // 手动调用 ItemBar 的滚轮处理
        return true; // 阻止事件继续传播
    }
    return QMainWindow::eventFilter(watched, event);
}

// ==================== 程序入口 ====================
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // 设置抗锯齿（可选）
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
