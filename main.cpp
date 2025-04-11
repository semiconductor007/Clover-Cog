#include <QApplication>
#include "rpg.h"
#include "mapwidget.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MapWidget mapWindow;
    mapWindow.loadMap(":/map/map5.tmj");

    // 创建角色控件并添加到地图窗口
    ImageMovementWidget *character = new ImageMovementWidget(&mapWindow);
    character->move(100, 100);
  character->setInitialPosition(QPoint(100, 100)); // 添加新方法

    mapWindow.show();
    return app.exec();
}
