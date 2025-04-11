

#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QWidget>
#include <QVector>
#include <QPixmap>
#include <QFileInfo>

struct Tileset {
    QPixmap texture;
    int firstGid;
    int tileWidth;
    int tileHeight;
    int tilesPerRow;  // 每行的图块数量
    QString sourcePath;
};

class MapWidget : public QWidget {
    Q_OBJECT
public:
    explicit MapWidget(QWidget *parent = nullptr);
    void loadMap(const QString &tmjPath); // 加载 .tmj 地图
    int tileWidth() const { return m_tileWidth; }
    int tileHeight() const { return m_tileHeight; }
    const QVector<QVector<int>>& collisionLayer() const { return m_collisionLayer; }


protected:
    void paintEvent(QPaintEvent *event) override;

private:
   QVector<QVector<QVector<int>>> m_mapLayers;   // 地图数据
    QPixmap m_tileset;                 // 图块集图片
    int m_tileWidth = 0;               // 图块宽度（像素）
    int m_tileHeight = 0;              // 图块高度
    int m_mapWidth = 0;                // 地图宽度（单位：图块）
    int m_mapHeight = 0;               // 地图高度
    int m_firstGid = 1;                // 第一个图块的全局ID
    QVector<Tileset> m_tilesets;


    QVector<QVector<int>> m_collisionLayer; // 新增碰撞层存储
};

#endif // MAPWIDGET_H
