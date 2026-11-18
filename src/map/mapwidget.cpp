#include "mapwidget.h"
#include "player/playercontroller.h"

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

MapWidget::MapWidget(QWidget *parent)
    : QWidget(parent) {
    setAttribute(Qt::WA_InputMethodEnabled, false);
    m_treeVisualTimer.setInterval(16);
    connect(&m_treeVisualTimer,&QTimer::timeout,this,[this]{
        for(int i=m_treeVisuals.size()-1;i>=0;--i)
            if(m_treeVisuals[i].elapsed.elapsed()>=(m_treeVisuals[i].falling?1040:220)) m_treeVisuals.removeAt(i);
        if(m_treeVisuals.isEmpty()) m_treeVisualTimer.stop();
        update();
    });
    m_chestTimer.setInterval(16);
    connect(&m_chestTimer, &QTimer::timeout, this, &MapWidget::advanceStorageChest);
}

void MapWidget::setStorageChestOpen(bool open) {
    if (m_chestTargetOpen == open) return;
    m_chestTargetOpen = open;
    if (!open && m_chestFrame == 0) {
        m_chestTimer.stop();
        return;
    }
    m_chestStartFrame = m_chestFrame;
    m_chestElapsed.start();
    m_chestTimer.start();
    emit storageChestCue(open);
}

void MapWidget::resetStorageChest() {
    m_chestTimer.stop();
    m_chestTargetOpen = false;
    m_chestFrame = m_chestStartFrame = 0;
    update();
}

void MapWidget::advanceStorageChest() {
    // Five front-facing frames in Sprout's atlas; use real elapsed time, not FPS.
    constexpr int FrameMs = 80;
    constexpr int LastFrame = 4;
    const int steps = int(m_chestElapsed.elapsed() / FrameMs);
    m_chestFrame = m_chestTargetOpen ? qMin(LastFrame, m_chestStartFrame + steps)
                                    : qMax(0, m_chestStartFrame - steps);
    update();
    const int duration = m_chestTargetOpen ? (LastFrame - m_chestStartFrame + 1) * FrameMs
                                         : qMax(1, m_chestStartFrame) * FrameMs;
    if (m_chestElapsed.elapsed() < duration) return;
    m_chestTimer.stop();
    if (m_chestTargetOpen) emit storageChestOpened();
}

void MapWidget::setPlayerController(PlayerController *controller) {
    m_playerController = controller;
}

bool MapWidget::loadMap(const QString &tmjPath) {
    MapData data;
    QString errorMessage;
    if (!MapLoader::load(tmjPath, &data, &errorMessage)) {
        qWarning() << errorMessage;
        emit mapLoadFailed(errorMessage);
        return false;
    }

    m_tileWidth = data.tileWidth;
    resetStorageChest();
    m_tileHeight = data.tileHeight;
    m_mapWidth = data.mapWidth;
    m_mapHeight = data.mapHeight;
    m_worldState.initialize(data);
    m_originalTilesets = std::move(data.tilesets);
    m_displayTilesets = m_originalTilesets;
    m_paintScenery = {};
    m_renderLayers = std::move(data.renderLayers);
    m_offsetX = 0;
    m_offsetY = 0;

    updateGeometry();
    update();
    return true;
}

bool MapWidget::hasTreeAt(const QPoint &tilePos) const {
    return m_worldState.hasTreeAt(tilePos);
}

bool MapWidget::modifyTreeHealth(const QPoint &tilePos, int damage) {
    if (!m_worldState.damageTree(tilePos, damage)) {
        return false;
    }
    update();
    return true;
}

void MapWidget::setSeasonAppearance(QVector<Tileset> displayTilesets,
                                    std::function<void(QPainter &, const QPoint &, const WorldState &)> paintScenery) {
    m_displayTilesets = displayTilesets.isEmpty() ? m_originalTilesets : std::move(displayTilesets);
    m_paintScenery = std::move(paintScenery);
    update();
}

void MapWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    m_renderer.paint(painter, rect(), QPoint(m_offsetX, m_offsetY),
                     m_mapWidth, m_mapHeight, m_tileWidth, m_tileHeight,
                     m_displayTilesets, m_renderLayers, m_worldState,
                     m_storageArea.isEmpty() ? QPoint(-1, -1) : m_storageArea.topLeft(),
                     m_chestFrame);
    if (m_paintScenery) {
        painter.save();
        m_paintScenery(painter, QPoint(m_offsetX, m_offsetY), m_worldState);
        painter.restore();
    }


    for(const auto &visual:m_treeVisuals) {
        const qreal progress=qMin(1.0,visual.elapsed.elapsed()/1040.0);
        const QPoint base=visual.anchor*16+QPoint(8,16)-QPoint(m_offsetX,m_offsetY);
        painter.save();
        painter.translate(base);
        if(!visual.falling) {
            painter.setPen(Qt::NoPen);painter.setBrush(QColor("#c89863"));
            const qreal amount=visual.elapsed.elapsed()/220.0;
            for(int i=0;i<5;++i) painter.drawRect(QRectF((i-2)*amount*8,-8-12*qSin(amount*3.14)+i,2,2));
            painter.restore();continue;
        }
        if(visual.useFallSheet) {
            static const QPixmap frames(QStringLiteral(":/farm-ui/tree-fall.png"));
            const int frame=qMin(12,int(visual.elapsed.elapsed()/80));
            if(visual.fallRight) painter.scale(-1,1);
            painter.drawPixmap(QRect(-32,-48,64,48),frames,QRect(frame*64,0,64,48));
            painter.restore();continue;
        }
        if(visual.falling) {
            painter.rotate((visual.fallRight?85:-85)*progress*progress);
            painter.setOpacity(1.0-qMax(0.0,(progress-0.75)*4));
        } else painter.translate(qSin(visual.elapsed.elapsed()*0.06)*1.5,0);
        painter.drawPixmap(QPoint(-visual.texture.width()/2,-visual.texture.height()),visual.texture);
        painter.restore();
    }
}

void MapWidget::setViewportCenter(int x, int y) {
    const int mapPixelWidth = m_mapWidth * m_tileWidth;
    const int mapPixelHeight = m_mapHeight * m_tileHeight;
    const int maxOffsetX = qMax(0, mapPixelWidth - width());
    const int maxOffsetY = qMax(0, mapPixelHeight - height());

    m_offsetX = qBound(0, qMax(0, x) - width() / 2, maxOffsetX);
    m_offsetY = qBound(0, qMax(0, y) - height() / 2, maxOffsetY);
    update();
}

void MapWidget::mousePressEvent(QMouseEvent *event) {
    event->accept();
    if (m_inputBlocked || event->button() != Qt::LeftButton)
        return;

    const QPointF local = event->position();
    const QPoint point(qFloor(local.x()), qFloor(local.y()));
    const QPoint tilePos = mapPointToTile(point);
    if (!isValidTile(tilePos))
        return;

    // Validate before harvesting or dispatching tools: neither inventory nor
    // world state may change for a remote click, including a mature crop.
    if (!m_playerController || !m_playerController->canReachTile(tilePos)) {
        emit farmingFeedback(QStringLiteral("距离太远，请靠近到周围一格内操作"));
        return;
    }

    // Fertilizer always goes through the crop and inventory checks, even on ripe crops.
    if (m_playerController && m_playerController->currentTool().type == ToolType::Fertilizer) {
        emit mapClicked(point);
        return;
    }

    if(m_playerController && m_playerController->currentTool().type==ToolType::Axe) {
        emit mapClicked(point); return;
    }

    // For other tools a mature crop owns the click, even when the backpack is full.
    const HarvestResult preview = m_worldState.previewHarvest(tilePos);
    if (preview.isValid()) {
        if (m_harvestHandler && !m_harvestHandler(preview.type, preview.quantity)) {
            emit harvestBlocked(preview.type, preview.quantity);
            return;
        }
        const HarvestResult harvested = m_worldState.harvest(tilePos);
        emit cropHarvested(harvested.type, harvested.quantity);
        emit worldChanged();
        update();
        return;
    }
    emit mapClicked(point);
}

QPoint MapWidget::mapPointToTile(const QPoint &mapPoint) const {
    if (m_tileWidth <= 0 || m_tileHeight <= 0 || !rect().contains(mapPoint))
        return QPoint(-1, -1);

    const QPoint world = mapPoint + QPoint(m_offsetX, m_offsetY);
    const QPoint tile(world.x() / m_tileWidth, world.y() / m_tileHeight);
    return isValidTile(tile) ? tile : QPoint(-1, -1);
}

bool MapWidget::isValidTile(const QPoint &tilePos) const {
    return m_worldState.isValidTile(tilePos);
}

int MapWidget::tileAt(int x, int y) const {
    if (x < 0 || x >= m_mapWidth || y < 0 || y >= m_mapHeight ||
        m_renderLayers.isEmpty()) {
        return -1;
    }

    const QVector<QVector<int>> &layer = m_renderLayers.first().tiles;
    if (y >= layer.size() || x >= layer.at(y).size()) {
        return -1;
    }
    return layer.at(y).at(x);
}

bool MapWidget::isInteractionTile(const QPoint &tilePos) const {
    return m_marketArea.contains(tilePos) || m_shopArea.contains(tilePos) ||
           m_bedArea.contains(tilePos) || m_storageArea.contains(tilePos);
}

bool MapWidget::cultivateTile(const QPoint &tilePos) {
    if (isInteractionTile(tilePos)) {
        emit farmingFeedback(QStringLiteral("互动区域不能耕种，请选草地"));
        return false;
    }
    if (m_worldState.cultivate(tilePos)) {
        update();
        emit farmingFeedback(QStringLiteral("已翻土，可以播种"));
        emit worldChanged();
        return true;
    }

    if (m_worldState.isPlantable(tilePos))
        emit farmingFeedback(QStringLiteral("这里可以直接播种"));
    else if (m_worldState.isPathTile(tilePos))
        emit farmingFeedback(QStringLiteral("道路不能耕种，请选草地"));
    else if (m_worldState.cropGidAt(tilePos) != 0)
        emit farmingFeedback(QStringLiteral("这里已经种有作物"));
    else
        emit farmingFeedback(QStringLiteral("这里不能耕种，请选草地"));
    return false;
}

bool MapWidget::isTileCultivatable(const QPoint &tilePos) const {
    return !isInteractionTile(tilePos) && m_worldState.isTileCultivatable(tilePos);
}

bool MapWidget::isPlantable(const QPoint &tilePos) const {
    return !isInteractionTile(tilePos) && m_worldState.isPlantable(tilePos);
}

bool MapWidget::plantSeed(const QPoint &tilePos, CropType cropType) {
    const qint64 gameMinute = m_getGameMinutes ? m_getGameMinutes() : 0;
    const int gameDay = static_cast<int>(gameMinute / (24 * 60));
    if (!isPlantable(tilePos)) {
        return false;
    }
    if (!CropCatalog::canPlantOnDay(cropType, gameDay)) {
        emit plantingOutOfSeason(cropType);
        return false;
    }
    const auto plantAction = [this, tilePos, cropType, gameDay]() {
        return m_worldState.plantSeed(tilePos, cropType, gameDay);
    };
    if (m_plantHandler && !m_plantHandler(cropType, plantAction)) {
        emit plantingBlocked(cropType);
        return false;
    }
    if (!m_plantHandler && !plantAction()) {
        return false;
    }
    update();
    emit worldChanged();
    return true;
}

bool MapWidget::fertilizeTile(const QPoint &tilePos) {
    WorldState candidate = m_worldState;
    FertilizeResult result;
    bool attempted = false;
    // The inventory owner checks stock, calls the effect synchronously, then
    // consumes one item only if it succeeded. Notify observers after both writes.
    const auto apply = [&]() {
        if (attempted)
            return false;
        attempted = true;
        result = candidate.fertilize(tilePos);
        return result.success;
    };
    if (m_fertilizeHandler && m_fertilizeHandler(apply) && result.success) {
        m_worldState = std::move(candidate);
        update();
        emit worldChanged();
        emit farmingFeedback(QStringLiteral("已施肥，下次浇水结算时加快生长"));
        return true;
    }
    QString reason;
    if (!attempted) {
        reason = QStringLiteral("肥料不足，请到商店购买");
    } else {
        switch (result.failure) {
        case FertilizeFailure::InvalidTile: reason = QStringLiteral("此处不能施肥"); break;
        case FertilizeFailure::NoCrop: reason = QStringLiteral("请对未成熟的作物施肥"); break;
        case FertilizeFailure::AlreadyApplied: reason = QStringLiteral("这株作物已经施过肥"); break;
        case FertilizeFailure::AlreadyMature: reason = QStringLiteral("作物已成熟，可以收获"); break;
        case FertilizeFailure::None: reason = QStringLiteral("施肥失败"); break;
        }
    }
    emit farmingFeedback(reason);
    return false;
}

bool MapWidget::waterTile(const QPoint &tilePos) {
    if (isInteractionTile(tilePos) || m_worldState.isWatered(tilePos) ||
        !m_worldState.waterTile(tilePos)) {
        return false;
    }
    update();
    emit worldChanged();
    emit tileWatered();
    return true;
}

DayAdvanceResult MapWidget::advanceDay(int completedDay,
                                       std::function<int()> randomPercent) {
    const DayAdvanceResult result =
        m_worldState.advanceDay(completedDay, std::move(randomPercent));
    update();
    return result;
}

DayAdvanceResult MapWidget::advanceThroughDay(int completedDay) {
    const auto result = m_worldState.advanceThroughDay(completedDay);
    update();
    return result;
}

bool MapWidget::chopTree(const QPoint &tile) {
    if(m_inputBlocked || !m_playerController || m_playerController->currentTool().type!=ToolType::Axe ||
       !m_playerController->canReachTile(tile)) return false;
    const auto preview=m_worldState.previewTreeChop(tile);
    if(!preview.success) {emit farmingFeedback(QStringLiteral("靠近树干，用斧头点击树干底部"));return false;}
    for(const auto &visual:m_treeVisuals) if(visual.anchor==preview.anchor && visual.falling) return false;
    const int crownGid=m_worldState.treeGidAt(preview.bounds.topLeft());
    QPixmap texture(preview.bounds.width()*16,preview.bounds.height()*16);texture.fill(Qt::transparent);
    {QPainter p(&texture);
        for(const auto &set:m_displayTilesets) if(set.sourcePath.endsWith(QStringLiteral("Trees, stumps and bushes.tsx")))
            for(int y=preview.bounds.top();y<=preview.bounds.bottom();++y)
                for(int x=preview.bounds.left();x<=preview.bounds.right();++x) {
                    int id=m_worldState.treeGidAt({x,y})-set.firstGid;
                    if(id>=0) p.drawPixmap(QRect((x-preview.bounds.x())*16,(y-preview.bounds.y())*16,16,16),set.texture,
                        QRect(id%12*16,id/12*16,16,16));
                }
    }
    const auto action=[this,preview]{return m_worldState.commitTreeChop(preview);};
    if(!m_woodHandler || !m_woodHandler(preview.wood,action)) {
        emit farmingFeedback(QStringLiteral("背包没有空格容纳木材，请先整理或存箱"));return false;
    }
    {
        TreeVisual visual;visual.anchor=preview.anchor;visual.bounds=preview.bounds;
        visual.texture=texture;visual.elapsed.start();visual.falling=preview.felled;
        visual.fallRight=m_playerController->playerTilePosition().x()<=preview.anchor.x();
        visual.useFallSheet=preview.felled && texture.size()==QSize(32,32) &&
            crownGid==336 && SeasonCalendar::dateForMinutes(m_getGameMinutes?m_getGameMinutes():0).season==Season::Spring;
        m_treeVisuals.append(visual);
        m_treeVisualTimer.start();
    }
    emit treeChopped(preview.felled);emit worldChanged();update();return true;
}
