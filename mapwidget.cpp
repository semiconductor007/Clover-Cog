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
    qDebug()<<m_tileWidth<<"  "<<m_tileHeight<<"hhhhhhhhhh";
    m_mapWidth = map["width"].toInt();          // 地图宽度（图块数量）
    m_mapHeight = map["height"].toInt();        // 地图高度（图块数量）

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
            // 获取图层尺寸（必须与地图尺寸一致）
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
            qDebug() << "===== 碰撞层数据样本 =====";
            for (int y = 0; y < 5 && y < m_collisionLayer.size(); ++y) {
                QString line;
                for (int x = 0; x < 5 && x < m_collisionLayer[y].size(); ++x) {
                    line += QString::number(m_collisionLayer[y][x]) + " ";
                }
                qDebug() << line;
            }

                m_mapLayers.append(layerData);

        }


    }

    update();
    setFixedSize(m_mapWidth * m_tileWidth, m_mapHeight * m_tileHeight);
    updateGeometry();
    qDebug() << "===== 碰撞层数据样本 =====";
    for (int y = 0; y < 5 && y < m_collisionLayer.size(); ++y) {
        QString line;
        for (int x = 0; x < 5 && x < m_collisionLayer[y].size(); ++x) {
            line += QString::number(m_collisionLayer[y][x]) + " ";
        }
        qDebug() << line;
    }
}

void MapWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    // 遍历所有图层（从底层到顶层）
    for (const auto &layerData : m_mapLayers) {
        for (int y = 0; y < m_mapHeight; y++) {
            for (int x = 0; x < m_mapWidth; x++) {
                int gid = layerData[y][x];
                if (gid == 0) continue;

                for (int i = m_tilesets.size() - 1; i >= 0; --i) {
                    const Tileset &tileset = m_tilesets[i];
                    if (gid >= tileset.firstGid) {
                        int localId = gid - tileset.firstGid;
                        int srcX = (localId % tileset.tilesPerRow) * tileset.tileWidth;
                        int srcY = (localId / tileset.tilesPerRow) * tileset.tileHeight;

                        painter.drawPixmap(
                            x * tileset.tileWidth,
                            y * tileset.tileHeight,
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
    // 调试：在正常图层上叠加绘制碰撞层
    if (!m_collisionLayer.isEmpty()) {
        painter.setBrush(QColor(255, 0, 0, 100)); // 半透明红色
        painter.setPen(Qt::NoPen);

        for (int y = 0; y < m_collisionLayer.size(); ++y) {
            for (int x = 0; x < m_collisionLayer[y].size(); ++x) {
                if (m_collisionLayer[y][x] != 0) {
                    QRect tileRect(
                        x * m_tileWidth,
                        y * m_tileHeight,
                        m_tileWidth,
                        m_tileHeight
                        );
                    painter.drawRect(tileRect);
                }
            }
        }
    }


      QWidget::paintEvent(event); // 确保子控件被绘制
    // mapwidget.cpp 中加载碰撞层后添加检查

}
