#include "mapwidget.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPainter>
#include <QDebug>
#include <QFileInfo>

MapWidget::MapWidget(QWidget *parent) : QWidget(parent) {}

void MapWidget::loadMap(const QString &tmjPath) {

    QFile file(tmjPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open map file:" << tmjPath;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) {
        qDebug() << "Invalid JSON file";
        return;
    }

    QJsonObject map = doc.object();

    // 读取地图基本信息
    m_tileWidth = map["tilewidth"].toInt();     // 每个图块的宽度（像素）
    m_tileHeight = map["tileheight"].toInt();   // 每个图块的高度（像素）
    m_mapWidth = map["width"].toInt();          // 地图宽度（图块数量）
    m_mapHeight = map["height"].toInt();        // 地图高度（图块数量）
    m_offsetX = 0;
    m_offsetY = 0;




    // 清空旧数据
    m_tilesets.clear();
    m_mapLayers.clear();


    // 加载所有图块集
    QJsonArray tilesets = map["tilesets"].toArray();
    for (const QJsonValue &tilesetValue : tilesets) {
        QJsonObject ts = tilesetValue.toObject();
        Tileset tileset;
        tileset.firstGid = ts["firstgid"].toInt();
        QString sourcePath = ts["source"].toString();

        // 生成图片路径
        QString imageName = QFileInfo(sourcePath).fileName().replace(".tsx", ".png");
        QString imagePath = ":/tilesets/sprout/" + imageName;

        if (!tileset.texture.load(imagePath)) {
            qDebug() << "Failed to load tileset:" << imagePath;
            continue;
        }



        tileset.tileWidth = m_tileWidth;          // 明确使用全局值
        tileset.tileHeight = m_tileHeight;
        tileset.tilesPerRow = tileset.texture.width() / m_tileWidth;
        m_tilesets.append(tileset);
    }

    // 加载所有图层
    QJsonArray layers = map["layers"].toArray();
    for (const QJsonValue &layerValue : layers) {
        QJsonObject layer = layerValue.toObject();
        QString layerName = layer["name"].toString();
        if (layer["type"].toString() == "tilelayer") {

            // 非无限状态下获取图层尺寸（必须与地图尺寸一致）
            int layerWidth = layer["width"].toInt();
            int layerHeight = layer["height"].toInt();

            // 解析图层数据
            QJsonArray dataArray = layer["data"].toArray();
            if (dataArray.size() != layerWidth * layerHeight) {
                qWarning() << "Layer" << layerName << "data size mismatch!";
                continue;
            }

            // 填充二维数据
            QVector<QVector<int>> layerData;
            layerData.resize(layerHeight);
            for (int y = 0; y < layerHeight; ++y) {
                layerData[y].resize(layerWidth);
                for (int x = 0; x < layerWidth; ++x) {
                    int index = y * layerWidth + x;
                    layerData[y][x] = dataArray[index].toInt();
                }
            }
            // 修改图层判断逻辑（兼容 Unicode 转义）
            if (layerName.toLower().contains("collision")) { // 更宽松的匹配
                m_collisionLayer = layerData;
                qDebug() << "已加载碰撞层，数据大小:" << m_collisionLayer.size() << "x" << m_collisionLayer[0].size();
            }

            for (int y = 0; y < 5 && y < m_collisionLayer.size(); ++y) {
                QString line;
                for (int x = 0; x < 5 && x < m_collisionLayer[y].size(); ++x) {
                    line += QString::number(m_collisionLayer[y][x]) + " ";
                }

            }

                m_mapLayers.append(layerData);

        }


    }

    update();
    updateGeometry();

    for (int y = 0; y < 5 && y < m_collisionLayer.size(); ++y) {
        QString line;
        for (int x = 0; x < 5 && x < m_collisionLayer[y].size(); ++x) {
            line += QString::number(m_collisionLayer[y][x]) + " ";
        }

    }
}

void MapWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    // 计算视口覆盖的图块范围（基于偏移量）
    int startTileX = m_offsetX / m_tileWidth;
    int startTileY = m_offsetY / m_tileHeight;
    int endTileX = qMin((m_offsetX + width()) / m_tileWidth + 1, m_mapWidth);
    int endTileY = qMin((m_offsetY + height()) / m_tileHeight + 1, m_mapHeight);

    // 遍历所有图层
    for (const auto& layerData : m_mapLayers) {
        for (int y = startTileY; y < endTileY; ++y) {
            for (int x = startTileX; x < endTileX; ++x) {
                int gid = layerData[y][x];
                if (gid == 0) continue;

                // 计算屏幕坐标（相对视口偏移）
                int screenX = x * m_tileWidth - m_offsetX;
                int screenY = y * m_tileHeight - m_offsetY;


                for (int i = m_tilesets.size() - 1; i >= 0; --i) {
                    const Tileset &tileset = m_tilesets[i];
                    if (gid >= tileset.firstGid) {
                        int localId = gid - tileset.firstGid;
                        int srcX = (localId % tileset.tilesPerRow) * tileset.tileWidth;
                        int srcY = (localId / tileset.tilesPerRow) * tileset.tileHeight;

                        painter.drawPixmap(
                            screenX,
                            screenY,
                            tileset.tileWidth,
                            tileset.tileHeight,
                            tileset.texture,
                            srcX, srcY,
                            tileset.tileWidth,
                            tileset.tileHeight
                            );
                        break;
                    }
                }
            }
        }
    }


    QWidget::paintEvent(event); // 确保子控件被绘制
    // 调试：在正常图层上叠加绘制碰撞层
    if (!m_collisionLayer.isEmpty()) {
        painter.setBrush(QColor(255, 0, 0, 100)); // 半透明红色
        painter.setPen(Qt::NoPen);

        for (int y = 0; y < m_collisionLayer.size(); ++y) {
            for (int x = 0; x < m_collisionLayer[y].size(); ++x) {
                if (m_collisionLayer[y][x] != 0) {
                    QRect tileRect(
                        (x * m_tileWidth) - m_offsetX,  // 减去视口偏移
                        (y * m_tileHeight) - m_offsetY,
                        m_tileWidth,
                        m_tileHeight
                        );
                    painter.drawRect(tileRect);
                }
            }
        }
    }


    // mapwidget.cpp 中加载碰撞层后添加检查

}

void MapWidget::setViewportSize(int width, int height) {
    m_viewportWidth = width;
    m_viewportHeight = height;
    update();
}

void MapWidget::setViewportCenter(int x, int y) {
    // 计算地图总像素尺寸
    int mapPixelWidth = m_mapWidth * m_tileWidth;
    int mapPixelHeight = m_mapHeight * m_tileHeight;

    // 计算最大允许偏移（确保不显示黑边）
    int maxOffsetX = qMax(0, mapPixelWidth - width());
    int maxOffsetY = qMax(0, mapPixelHeight - height());

    // 计算新偏移（让角色位于视口中央）
    int newOffsetX = x - width() / 2;
    int newOffsetY = y - height() / 2;

    // 约束偏移范围
    m_offsetX = qBound(0, newOffsetX, maxOffsetX);
    m_offsetY = qBound(0, newOffsetY, maxOffsetY);

    update(); // 触发重绘
}


