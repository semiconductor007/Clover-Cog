
//mapwidget.h
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
    void setViewportCenter(int x, int y); // 设置视口中心点
    void setViewportSize(int width, int height);
    int mapWidth() const { return m_mapWidth; }
    int mapHeight() const { return m_mapHeight; }

    int m_offsetX = 0; // 视口左上角在地图中的 X 偏移
    int m_offsetY = 0; // 视口左上角在地图中的 Y 偏移
    int offsetX() const { return m_offsetX; }
    int offsetY() const { return m_offsetY; }
    int viewportWidth() const { return width(); }
    int viewportHeight() const { return height(); }
    bool hasTreeAt(const QPoint& tilePos) const;
    bool modifyTreeHealth(const QPoint& tilePos, int damage);
    
    // Add this declaration
    int tileAt(int x, int y) const;

    void cultivateTile(int x, int y);
    bool isTileCultivatable(int x, int y, int grassGid) const;
    void setTimeReference(std::function<QPair<int,int>()> timeGetter) {
        m_getGameTime = timeGetter;
    }
    void cropTile(int x, int y);
    bool isPlantable(int x, int y, int cultivateGid);
    void plantSeed(const QPoint& tilePos);
    enum class CropType {
        None,
        Wheat,
        Potato
    };






protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent* event) override;


private:
    // 私有成员变量必须在类定义内部声明
    std::function<QPair<int,int>()> m_getGameTime;
    QVector<QVector<QVector<int>>> m_mapLayers;   
    QPixmap m_tileset;
    int m_tileWidth = 0;
    int m_tileHeight = 0;
    int m_mapWidth = 0;
    int m_mapHeight = 0;
    int m_firstGid = 1;
    QVector<Tileset> m_tilesets;
    int m_viewportWidth = 800; // 固定视口宽度
    int m_viewportHeight = 600; // 固定视口高度
    QVector<QVector<int>> m_collisionLayer; // 新增碰撞层存储
    struct TreeInfo {
        QPoint tilePos;
        int health;
        bool interactable;
        bool cultivable = false;    // 是否可耕作
        QString cropType;           // 作物类型
        int growthStage = 0;        // 生长阶段
    };
    QHash<QPoint, TreeInfo> m_trees; // 使用QHash存储树木数据
    QMap<int, TreeInfo> m_tilesetProperties; // 存储图块属性

    QVector<QVector<int>> m_cultivatableLayer; // 可耕作图层
    QVector<QVector<int>> m_cultivatedLayer;   // 耕地图层

    void updateCultivatedTile(int x, int y);

    QVector<QVector<int>> m_cropLayer; // 新增作物图层
    QHash<QPoint, QDateTime> m_plantingTime; // 种植时间记录
    QHash<QPoint, int> m_growthStages; // 生长阶段记录

    int calculateGrowthStage(const QPoint& pos);
    void updateGrowthStages();
    QTimer* m_growthTimer; // 生长更新定时器
     const QSet<int> m_plantableGids = {123, 124, 125, 134,135 , 136, 145, 146, 147};




signals:  // 新增信号声明
    void mapClicked(QPoint pos);  // 声明点击信号
     void cropHarvested(MapWidget::CropType type);








};



#endif // MAPWIDGET_H
