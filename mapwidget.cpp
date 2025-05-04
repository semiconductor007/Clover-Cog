//mapwidget.cpp
#include "mapwidget.h"
#include "tooltype.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPainter>
#include <QDebug>
#include <QFileInfo>
#include <QMouseEvent>
#include <QXmlStreamReader>
#include <QTimer>

MapWidget::MapWidget(QWidget *parent) : QWidget(parent) {
setAttribute(Qt::WA_InputMethodEnabled, false);

    // 添加生长定时器（每秒检查一次）
    m_growthTimer = new QTimer(this);
    connect(m_growthTimer, &QTimer::timeout, this, &MapWidget::updateGrowthStages);
    m_growthTimer->start(1000); // 每秒触发一次

}

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

    // 处理树木层

    QByteArray data = file.readAll();
    QJsonObject mapObject = doc.object();  // Add this declaration

    QJsonArray layersArray = mapObject["layers"].toArray();
    for (const auto& layerValue : layersArray) {
        QJsonObject layerObj = layerValue.toObject();
        if (layerObj["name"].toString() == "tree" && layerObj["type"].toString() == "tilelayer") {
            QJsonArray dataArray = layerObj["data"].toArray();
            int dataSize = dataArray.size();
            int expectedSize = m_mapWidth * m_mapHeight;

            // 添加数据完整性检查
            if (dataSize < expectedSize) {
                qWarning() << "Tree layer data incomplete. Expected:" << expectedSize
                           << "Actual:" << dataSize;
                break; // 跳过不完整的数据
            }

            for (int y = 0; y < m_mapHeight; ++y) {
                for (int x = 0; x < m_mapWidth; ++x) {
                    int index = y * m_mapWidth + x;
                    if (index >= dataSize) break; // 防止越界

                    int tileId = dataArray[index].toInt();
                    if (m_tilesetProperties.contains(tileId)) {
                        TreeInfo tree = m_tilesetProperties[tileId];
                        tree.tilePos = QPoint(x, y);
                        m_trees.insert(tree.tilePos, tree);
                    }
                }
            }
        }
    }



    // 加载所有图块集
    QJsonArray tilesets = map["tilesets"].toArray();
    for (const QJsonValue &tilesetValue : tilesets) {
        QJsonObject ts = tilesetValue.toObject();
        Tileset tileset;
        tileset.firstGid = ts["firstgid"].toInt();
        QString sourcePath = ts["source"].toString();

        // 动态加载.tsx属性文件
        QString tsxPath = ":/tilesets/tsx/" + QFileInfo(sourcePath).fileName();
        QFile tsxFile(tsxPath);
        if (tsxFile.open(QIODevice::ReadOnly)) {
            QXmlStreamReader xml(&tsxFile);
            while (!xml.atEnd()) {
                if (xml.readNextStartElement() && xml.name() == "tile") {
                    int localId = xml.attributes().value("id").toInt();
                    int globalId = tileset.firstGid + localId; // 使用当前tileset的firstgid
                    TreeInfo props;

                    while (xml.readNextStartElement()) {
                        if (xml.name() == "property") {
                            QString name = xml.attributes().value("name").toString();
                            QString value = xml.attributes().value("value").toString();

                            if (name == "health") props.health = value.toInt();
                            if (name == "interactable") props.interactable = (value == "true");
                            if (name == "type" && value == "tree") props.interactable = true;
                            if (name == "cultivable") props.cultivable = (value == "true");
                            if (name == "cropType") props.cropType = value;
                            if (name == "growthStage") props.growthStage = value.toInt();
                        }
                        xml.skipCurrentElement();
                    }
                    m_tilesetProperties[globalId] = props;
                }
            }
            tsxFile.close();
        }

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

            // 通用图层加载逻辑（新增边界检查）
            int layerWidth = layer["width"].toInt();
            int layerHeight = layer["height"].toInt();

            // 验证图层尺寸与地图尺寸一致
            if (layerWidth != m_mapWidth || layerHeight != m_mapHeight) {
                qWarning() << "Layer" << layerName << "尺寸不匹配，跳过加载";
                continue;
            }

            QJsonArray dataArray = layer["data"].toArray();
            QVector<QVector<int>> layerData;
            layerData.resize(m_mapHeight);
            for (int y = 0; y < m_mapHeight; ++y) {
                layerData[y].resize(m_mapWidth);
                for (int x = 0; x < m_mapWidth; ++x) {
                    int index = y * m_mapWidth + x;
                    // 新增越界检查
                    if (index >= 0 && index < dataArray.size()) {
                        layerData[y][x] = dataArray[index].toInt();
                    }
                }
            }

            // 特殊图层处理
            QString lowerName = layerName.toLower();
            if (lowerName.contains("collision")) {
                m_collisionLayer = layerData;
            } else if (lowerName == "cultivatable") {
                m_cultivatableLayer = layerData;
                m_mapLayers.append(layerData);
                // m_cultivatedLayer = QVector<QVector<int>>(m_mapHeight, QVector<int>(m_mapWidth, 0));
            }else if (lowerName =="cultivated"){
                m_cultivatedLayer=layerData;
                m_cultivatedLayer.resize(m_mapHeight);
                m_mapLayers.append(layerData);
            }
            else if (lowerName == "crop") {  // 新增作物层处理
                m_cropLayer = layerData;
            }
            else {
                m_mapLayers.append(layerData);    // 普通图层加入渲染队列
            }


            for (int y = 0; y < 5 && y < m_collisionLayer.size(); ++y) {
                QString line;
                for (int x = 0; x < 5 && x < m_collisionLayer[y].size(); ++x) {
                    line += QString::number(m_collisionLayer[y][x]) + " ";
                }

            }

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

    // 在加载地图后添加调试输出
    qDebug() << "Loaded trees:" << m_trees.size();
    for (auto it = m_trees.begin(); it != m_trees.end(); ++it) {
        qDebug() << "Tree at:" << it.key() << "Health:" << it.value().health;
    }
}

bool MapWidget::hasTreeAt(const QPoint& tilePos) const {
    // 在hasTreeAt方法中修正调试信息
    int tileId = tileAt(tilePos.x(), tilePos.y());
    TreeInfo props = m_tilesetProperties.value(tileId);

    qDebug() << "检查图块：" << tilePos
             << "生命值：" << props.health
             << "可交互：" << props.interactable
             << "树木存在：" << m_trees.contains(tilePos);
    return m_trees.contains(tilePos);
}

bool MapWidget::modifyTreeHealth(const QPoint& tilePos, int damage) {
    if (m_trees.contains(tilePos)) {
        TreeInfo& tree = m_trees[tilePos];
        tree.health -= damage;
        qDebug() << "树木位置：" << tilePos
                 << "受到伤害：" << damage
                 << "剩余生命：" << tree.health;
        if (tree.health <= 0) {
            m_trees.remove(tilePos);
            update();
            qDebug() << "树木已被砍伐";
        }
        return true;
    }
    return false;
}

void MapWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    // 计算视口覆盖的图块范围（基于偏移量）
    int startTileX = m_offsetX / m_tileWidth;
    int startTileY = m_offsetY / m_tileHeight;
    int endTileX = qMin((m_offsetX + width()) / m_tileWidth + 1, m_mapWidth);
    int endTileY = qMin((m_offsetY + height()) / m_tileHeight + 1, m_mapHeight);




    if (!m_cultivatableLayer.isEmpty()) {
        for (int y = startTileY; y < endTileY; ++y) {
            for (int x = startTileX; x < endTileX; ++x) {
                int gid = m_cultivatableLayer[y][x];
                if (gid == 0) continue;

                for (int i = m_tilesets.size() - 1; i >= 0; --i) {
                    const Tileset &tileset = m_tilesets[i];
                    if (gid >= tileset.firstGid) {
                        int localId = gid - tileset.firstGid;
                        int srcX = (localId % tileset.tilesPerRow) * tileset.tileWidth;
                        int srcY = (localId / tileset.tilesPerRow) * tileset.tileHeight;
                        int screenX = x * tileset.tileWidth - m_offsetX;
                        int screenY = y * tileset.tileHeight - m_offsetY;

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

    if (!m_cultivatedLayer.isEmpty()) {
        for (const Tileset& ts : m_tilesets) {
            if (135 >= ts.firstGid && 135 < ts.firstGid + ts.tilesPerRow * (ts.texture.height()/ts.tileHeight)) {
                int localId = 135 - ts.firstGid;
                int srcX = (localId % ts.tilesPerRow) * ts.tileWidth;
                int srcY = (localId / ts.tilesPerRow) * ts.tileHeight;

                // 绘制所有已耕作图块
                for (int y = startTileY; y < endTileY; ++y) {
                    for (int x = startTileX; x < endTileX; ++x) {
                        if (m_cultivatedLayer[y][x] == 135) {
                            int screenX = x * ts.tileWidth - m_offsetX;
                            int screenY = y * ts.tileHeight - m_offsetY;
                            painter.drawPixmap(screenX, screenY, ts.tileWidth, ts.tileHeight,
                                               ts.texture, srcX, srcY, ts.tileWidth, ts.tileHeight);
                        }
                    }
                }
                break;
            }
        }
    }

    if (!m_cropLayer.isEmpty()) {
        for (const Tileset& ts : m_tilesets) {
            // 修改判断条件，检查图块集是否包含作物GID范围
            if (ts.firstGid <= 243 && (ts.firstGid + ts.tilesPerRow*(ts.texture.height()/ts.tileHeight)) >= 253) {
                for (int y = startTileY; y < endTileY; ++y) {
                    for (int x = startTileX; x < endTileX; ++x) {
                        int currentGid = m_cropLayer[y][x];
                        if (currentGid < 240 || currentGid > 243) continue;

                        // 直接使用存储的 GID
                        int localId = currentGid - ts.firstGid;
                        int srcX = (localId % ts.tilesPerRow) * ts.tileWidth;
                        int srcY = (localId / ts.tilesPerRow) * ts.tileHeight;

                        painter.drawPixmap(
                            x * ts.tileWidth - m_offsetX,
                            y * ts.tileHeight - m_offsetY,
                            ts.tileWidth,
                            ts.tileHeight,
                            ts.texture,
                            srcX, srcY,
                            ts.tileWidth,
                            ts.tileHeight
                            );
                    }
                }
                break;
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

void MapWidget::mousePressEvent(QMouseEvent* event) {

    if(event->button() == Qt::LeftButton) {
        // 转换点击坐标到图块位置
        QPointF pos = event->position();
        int tileX = (pos.x() + m_offsetX) / m_tileWidth;
        int tileY = (pos.y() + m_offsetY) / m_tileHeight;

        if(m_cropLayer[tileY][tileX] == 243) { // 小麦成熟阶段应为243
            m_cropLayer[tileY][tileX] = 0;
            QPoint key(tileX, tileY);
            m_plantingTime.remove(key);                // 清除种植时间
            m_growthStages.remove(key);
            emit cropHarvested(MapWidget::CropType::Wheat);
            update();
        }
    }

    // 触发工具动作（需传递到角色控件）
    emit mapClicked(event->pos());  // 通过信号传递点击事件
    QWidget::mousePressEvent(event);
}

int MapWidget::tileAt(int x, int y) const {
    if (x >= 0 && x < m_mapWidth && y >= 0 && y < m_mapHeight) {
        // 修改为安全访问
        if (!m_mapLayers.isEmpty()) {
            return m_mapLayers.first()[y][x];
        }
    }
    return -1;
}


void MapWidget::updateCultivatedTile(int x, int y) {
    // 建议添加边界保护（防止越界崩溃）
    x = qBound(0, x, m_mapWidth - 1);
    y = qBound(0, y, m_mapHeight - 1);

    m_cultivatedLayer[y][x] = 135;
    qDebug() << "设置耕地图块GID:135 at (" << x << "," << y << ")";
}

void MapWidget::cultivateTile(int x, int y) {
    qDebug() << "耕作图块坐标：" << x << y;

    m_cultivatedLayer[y][x] = 135;
    qDebug() << "初始化耕地图块GID:135";
    update();
}

bool MapWidget::isTileCultivatable(int x, int y, int grassGid) const
{
    if (x >= 0 && x < m_mapWidth && y >= 0 && y < m_mapHeight) {
        qDebug() << "当前图块GID:" << m_cultivatableLayer[y][x]
                 << "目标GID:" << grassGid;
        return m_cultivatableLayer[y][x] == grassGid;
    }
    return false;
}

void MapWidget::cropTile(int x, int y) {

    if (x < 0 || x >= m_mapWidth || y < 0 || y >= m_mapHeight)
        return;

    m_cropLayer[y][x] = 240;
    update();
}

bool MapWidget::isPlantable(int x, int y, int plantGid)
{
    if (x < 0 || x >= m_mapWidth || y < 0 || y >= m_mapHeight) return false;

    // return (m_cultivatedLayer[y][x] == plantGid) &&
    //        (m_cropLayer[y][x] == 0);
    return m_plantableGids.contains(m_cultivatedLayer[y][x]) &&
           (m_cropLayer[y][x] == 0);
}

void MapWidget::plantSeed(const QPoint& tilePos) {
    // 添加边界检查
    if (tilePos.y() >= m_cultivatedLayer.size() ||
        tilePos.x() >= m_cultivatedLayer[tilePos.y()].size()) {
        qDebug() << "种植失败：坐标越界" << tilePos;
        return;
    }

    if (/*m_cultivatedLayer[tilePos.y()][tilePos.x()] == 135*/
        m_plantableGids.contains(m_cultivatedLayer[tilePos.y()][tilePos.x()])) {
        m_plantingTime[tilePos] = QDateTime::currentDateTime();
        m_cropLayer[tilePos.y()][tilePos.x()] = 240;
        update();

        qDebug() << "种植记录 - 位置:" << tilePos
                 << " 时间:" << m_plantingTime[tilePos].toString("hh:mm:ss")
                 << " 当前GID:" << m_cropLayer[tilePos.y()][tilePos.x()];
    } else {
        qDebug() << "种植失败：非耕地" << tilePos
                 << " 耕地GID:" << m_cultivatedLayer[tilePos.y()][tilePos.x()];
    }
}


int MapWidget::calculateGrowthStage(const QPoint& pos) {
    // qint64 elapsedMinutes = m_plantingTime[pos].secsTo(QDateTime::currentDateTime()) / 60;
    // 每18现实分钟推进一阶段（3天=72游戏小时=72现实分钟）
    // return qMin(elapsedMinutes / 18, 3);
    if (!m_plantingTime.contains(pos)) return 0;

    qint64 elapsedSeconds = m_plantingTime[pos].secsTo(QDateTime::currentDateTime());
    // 测试阶段：每10秒一个阶段（实际可按需调整）
    return qMin(elapsedSeconds / 10, 3LL);
}

void MapWidget::updateGrowthStages() {

    QMutableHashIterator<QPoint, QDateTime> it(m_plantingTime);
    while (it.hasNext()) {
        it.next();
        const QPoint& pos = it.key();

        // 边界检查
        if (pos.y() >= m_cropLayer.size() || pos.x() >= m_cropLayer[pos.y()].size()) {
            qDebug() << "无效坐标，跳过";
            continue;
        }

        int oldGid = m_cropLayer[pos.y()][pos.x()];
        int stage = calculateGrowthStage(pos);
        int newGid = 240 + qBound(0, stage, 3);

        if (newGid != oldGid) {
            m_cropLayer[pos.y()][pos.x()] = newGid;
            qDebug() << "更新作物 GID：" << oldGid << "→" << newGid
                     << "，阶段：" << stage;
            update();
        }
        if(m_cropLayer[pos.y()][pos.x()] == 0) {
            it.remove();
            continue;
        }
    }
}
