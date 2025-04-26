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


    connect(mapWidget, &MapWidget::mapClicked, [this](QPoint pos) {
        // 通过character获取当前工具
        const ToolProperty &tool = character->getCurrentTool();
            qDebug() << "=== 鼠标点击事件 ===";
        qDebug() << "点击位置:" << pos << "当前工具:" << tool.name;
        character->handleToolAction();
    });

    // 连接物品栏的索引变化信号到角色工具切换
    connect(itemBar, &ItemBar::itemSelected,
            character, &ImageMovementWidget::setCurrentTool);






     // 为窗口安装事件过滤器
    this->installEventFilter(this);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::Wheel) {
        itemBar->wheelEvent(static_cast<QWheelEvent*>(event));
        return true; // 拦截滚轮事件
    }

    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            qDebug() << "[MainWindow] 鼠标点击位置（窗口坐标）:" << mouseEvent->pos();

            // 获取地图控件的滚动偏移
            int mapOffsetX = mapWidget->offsetX();
            int mapOffsetY = mapWidget->offsetY();

            // 计算实际点击位置在地图中的全局坐标
            QPoint mapGlobalPos = mapWidget->mapToGlobal(QPoint(0, 0));
            QPoint actualMapPos = mouseEvent->pos() + QPoint(mapOffsetX, mapOffsetY);

            // 转换为角色控件的局部坐标
            QPoint characterLocalPos = character->mapFromGlobal(actualMapPos + mapGlobalPos);
            if (character->rect().contains(characterLocalPos)) {
                qDebug() << "点击在角色控件内";
                character->handleToolAction();
            }
            return false; // 允许事件继续传递
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

// ==================== 程序入口 ====================
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);



    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
