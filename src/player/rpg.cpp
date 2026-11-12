//rpg.cpp
#include "rpg.h"
#include "map/mapwidget.h"
#include <QDebug>
#include <QMouseEvent>
#include <QTransform>
#include <limits>

QPoint ImageMovementWidget::boundedPosition(QPoint pos) const {
    if (!m_mapWidget) return pos;
    const int maxX = qMax(0, m_mapWidget->mapWidth() * m_mapWidget->tileWidth() - width());
    const int maxY = qMax(0, m_mapWidget->mapHeight() * m_mapWidget->tileHeight() - height());
    return QPoint(qBound(0, pos.x(), maxX), qBound(0, pos.y(), maxY));
}

void ImageMovementWidget::setInitialPosition(const QPoint &pos) {
    m_absolutePos = boundedPosition(pos);
    if (m_mapWidget) {
        m_mapWidget->setViewportCenter(m_absolutePos.x() + width() / 2,
                                       m_absolutePos.y() + height() / 2);
        move(m_absolutePos - QPoint(m_mapWidget->offsetX(), m_mapWidget->offsetY()));
    }
}

bool ImageMovementWidget::restorePosition(const QPoint &saved) {
    if (!m_mapWidget || !m_collisionDetector || saved != boundedPosition(saved) ||
        m_mapWidget->mapWidth() <= 0 || m_mapWidget->mapHeight() <= 0)
        return false;

    if (!m_collisionDetector->isColliding(m_collisionArea.translated(saved))) {
        setInitialPosition(saved);
        return true;
    }

    // A map edit may add a fence at an otherwise valid saved position.
    const int tileWidth = m_mapWidget->tileWidth();
    const int tileHeight = m_mapWidget->tileHeight();
    const QPoint centerOffset(tileWidth / 2 - width() / 2, tileHeight / 2 - height() / 2);
    QPoint nearest;
    qint64 bestDistance = std::numeric_limits<qint64>::max();
    for (int y = 0; y < m_mapWidget->mapHeight(); ++y) {
        for (int x = 0; x < m_mapWidget->mapWidth(); ++x) {
            const QPoint candidate = QPoint(x * tileWidth, y * tileHeight) + centerOffset;
            if (boundedPosition(candidate) != candidate) continue;
            const qint64 dx = qint64(candidate.x()) - saved.x();
            const qint64 dy = qint64(candidate.y()) - saved.y();
            const qint64 distance = dx * dx + dy * dy;
            if (distance < bestDistance &&
                !m_collisionDetector->isColliding(m_collisionArea.translated(candidate))) {
                bestDistance = distance;
                nearest = candidate;
            }
        }
    }

    if (bestDistance == std::numeric_limits<qint64>::max()) return false;
    setInitialPosition(nearest);
    return true;
}

QPoint ImageMovementWidget::currentTilePosition() const {
    if (!m_mapWidget || m_mapWidget->tileWidth() <= 0 ||
        m_mapWidget->tileHeight() <= 0) {
        return QPoint(-1, -1);
    }
    const QPoint center = m_absolutePos + QPoint(width() / 2, height() / 2);
    return QPoint(center.x() / m_mapWidget->tileWidth(),
                  center.y() / m_mapWidget->tileHeight());
}


ImageMovementWidget::ImageMovementWidget(MapWidget *mapWidget, QWidget *parent)
    : QWidget(parent), m_mapWidget(mapWidget) {
    m_controller = new PlayerController(mapWidget, this);
    initUI();
    currentRightFrameIndex=0;
    currentLeftFrameIndex=0;
    currentUpFrameIndex=0;
    currentDownFrameIndex=0;
    currentIdleFrameIndex=0;
    isMovingRight = false;
    isMovingLeft = false;
    isMovingUp = false;
    isMovingDown = false;
    isIdle=true;

    animationTimer = new QTimer(this);
    connect(animationTimer, &QTimer::timeout, this, &ImageMovementWidget::updateAnimation);
    animationTimer->start(100); // 每100毫秒更新动画和位置

    // 初始化碰撞检测
    if (m_mapWidget) {
        m_mapWidget->setFocusPolicy(Qt::ClickFocus);
        m_mapWidget->setFocusProxy(this);
        m_collisionDetector = std::make_unique<CollisionDetector>(
            &m_mapWidget->collisionLayer(),
            m_mapWidget->tileWidth(),
            m_mapWidget->tileHeight()
            );
    }

    // MapWidget arbitrates harvesting and tool use for every left click.
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_AcceptTouchEvents, false);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

//切换工具
void ImageMovementWidget::setCurrentTool(int toolIndex) {
    m_controller->setCurrentTool(toolIndex);
}

void ImageMovementWidget::initUI() {
    setGeometry(300, 300, 800, 600);
    setWindowTitle("Image Movement with Keyboard");

    // 加载精灵图（确保路径正确）
    spriteSheet.load(":/characte/Premium Charakter Spritesheet.png");
    idleSheet.load(":/characte/Premium Charakter Spritesheet.png");

    int frameWidth = 48;
    int frameHeight = 48;


    // 初始化动作定时器
    m_actionTimer = new QTimer(this);
    connect(m_actionTimer, &QTimer::timeout, this, &ImageMovementWidget::updateActionAnimation);


    // 初始化锄头动画
    ToolAnimation hoeAnim;
    for(int i=0; i<6; ++i){
        hoeAnim.rightFrames.append(spriteSheet.copy(i*48, 14*48, 48, 48));
        hoeAnim.leftFrames.append(hoeAnim.rightFrames.last().transformed(QTransform().scale(-1, 1)));
        hoeAnim.upFrames.append(spriteSheet.copy(i*48, 13*48, 48, 48));
        hoeAnim.downFrames.append(spriteSheet.copy(i*48, 12*48, 48, 48));
    }
    m_toolAnimations[ToolType::Hoe] = hoeAnim;


    // 初始化浇水动画
    ToolAnimation wateringCanAnim;
    for(int i = 0; i < 6; ++i) {
        wateringCanAnim.downFrames.append(spriteSheet.copy(i*48, 20*48, 48, 48));
        wateringCanAnim.upFrames.append(spriteSheet.copy(i*48, 21*48, 48, 48));
        wateringCanAnim.rightFrames.append(spriteSheet.copy(i*48, 22*48, 48, 48));
        wateringCanAnim.leftFrames.append(wateringCanAnim.rightFrames.last().transformed(QTransform().scale(-1, 1)));
    }
    m_toolAnimations[ToolType::WateringCan] = wateringCanAnim;

    // 初始化空闲帧
    for (int i = 0; i < 8; ++i) {
        idleFrames.append(idleSheet.copy(i * frameWidth, 0, frameWidth, frameHeight));
        idleLeftFrames.append(idleSheet.copy(i * frameWidth, 3 * frameHeight, frameWidth, frameHeight));
        idleRightFrames.append(idleSheet.copy(i * frameWidth,2 * frameHeight, frameWidth, frameHeight));
        idleUpFrames.append(idleSheet.copy(i * frameWidth, 1 * frameHeight, frameWidth, frameHeight));
    }

    // 初始化各方向动画帧
    walkingRightFrames.clear();
    for (int i = 0; i < 8; ++i) {
        walkingRightFrames.append(spriteSheet.copy(i * frameWidth, 6 * frameHeight, frameWidth, frameHeight));
    }

    // 水平翻转生成向左走的帧
    for (const QPixmap& rightFrame : walkingRightFrames) {
        QPixmap leftFrame = rightFrame.transformed(QTransform().scale(-1, 1));
        walkingLeftFrames.append(leftFrame);
    }

    // 其他方向帧
    for (int i = 0; i < 8; ++i) {
        walkingUpFrames.append(spriteSheet.copy(i * frameWidth, 5 * frameHeight, frameWidth, frameHeight));
    }

    for (int i = 0; i < 8; ++i) {
        walkingDownFrames.append(spriteSheet.copy(i * frameWidth, 4 * frameHeight, frameWidth, frameHeight));
    }

    imageLabel = new QLabel(this);
    imageLabel->setPixmap(idleFrames.first());
    imageLabel->setAlignment(Qt::AlignCenter); // 居中显示
    this->setFixedSize(48, 48); // 固定控件尺寸为角色大小
    imageLabel->setFixedSize(48, 48); // 确保标签尺寸一致
    m_collisionArea = QRect(20, 24, 8, 8); // 中心16x16
    // 设置透明背景
    imageLabel->setAttribute(Qt::WA_TranslucentBackground);
    imageLabel->setAutoFillBackground(false);
}

void ImageMovementWidget::clearMovement() {
    isMovingUp = isMovingDown = isMovingLeft = isMovingRight = false;
    isIdle = true;
    currentIdleFrameIndex = 0;
}

void ImageMovementWidget::setInputBlocked(bool blocked) {
    if (m_inputBlocked == blocked)
        return;
    m_inputBlocked = blocked;
    clearMovement();
    if (blocked) {
        m_actionTimer->stop();
        m_isActing = false;
    } else {
        setFocus(Qt::OtherFocusReason);
    }
}

void ImageMovementWidget::focusOutEvent(QFocusEvent *event) {
    clearMovement();
    QWidget::focusOutEvent(event);
}

bool ImageMovementWidget::event(QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Tab && key->modifiers() == Qt::NoModifier) {
            key->accept();
            if (!m_inputBlocked && !key->isAutoRepeat())
                emit inventoryRequested();
            return true;
        }
    }
    return QWidget::event(event);
}

void ImageMovementWidget::keyPressEvent(QKeyEvent *event) {
    if (m_inputBlocked || event->isAutoRepeat()) {
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_E) {
        emit interactionRequested(currentTilePosition());
        event->accept();
        return;
    }

    switch (event->key()) {
    case Qt::Key_D:
        isMovingRight = true;
        isMovingLeft = false;
        isMovingUp = false;
        isMovingDown = false;
         m_lastDirection = PlayerDirection::Right;

        break;
    case Qt::Key_A:
        isMovingLeft = true;
        isMovingRight = false;
        isMovingUp = false;
        isMovingDown = false;
        m_lastDirection = PlayerDirection::Left;

        break;
    case Qt::Key_W:
        isMovingUp = true;
        isMovingRight = false;
        isMovingLeft = false;
        isMovingDown = false;
        m_lastDirection = PlayerDirection::Up;

        break;
    case Qt::Key_S:
        isMovingDown = true;
        isMovingRight = false;
        isMovingLeft = false;
        isMovingUp = false;
         m_lastDirection = PlayerDirection::Down;

        break;
    default:
        QWidget::keyPressEvent(event);
        return;
    }
    isIdle = false;
    event->accept();
}

void ImageMovementWidget::keyReleaseEvent(QKeyEvent *event) {
    if (m_inputBlocked || event->isAutoRepeat()) {
        event->accept();
        return;
    }

    switch (event->key()) {
    case Qt::Key_D:
        isMovingRight = false;

        break;
    case Qt::Key_A:
        isMovingLeft = false;

        break;
    case Qt::Key_W:
        isMovingUp = false;

        break;
    case Qt::Key_S:
        isMovingDown = false;

        break;
    default:
        QWidget::keyReleaseEvent(event);
        return;
    }

    // 所有方向都释放时切换到空闲状态
    if (!isMovingRight && !isMovingLeft && !isMovingUp && !isMovingDown) {
        isIdle = true;
        currentIdleFrameIndex = 0;
    }
    event->accept();
}

void ImageMovementWidget::updateAnimation() {
    if (m_inputBlocked || m_isActing || !m_collisionDetector || !m_mapWidget)
        return;

    constexpr int step = 10;
    QPoint next = m_absolutePos;
    if (isMovingRight) {
        next.rx() += step;
        currentRightFrameIndex = (currentRightFrameIndex + 1) % walkingRightFrames.size();
        imageLabel->setPixmap(walkingRightFrames[currentRightFrameIndex]);
    } else if (isMovingLeft) {
        next.rx() -= step;
        currentLeftFrameIndex = (currentLeftFrameIndex + 1) % walkingLeftFrames.size();
        imageLabel->setPixmap(walkingLeftFrames[currentLeftFrameIndex]);
    } else if (isMovingUp) {
        next.ry() -= step;
        currentUpFrameIndex = (currentUpFrameIndex + 1) % walkingUpFrames.size();
        imageLabel->setPixmap(walkingUpFrames[currentUpFrameIndex]);
    } else if (isMovingDown) {
        next.ry() += step;
        currentDownFrameIndex = (currentDownFrameIndex + 1) % walkingDownFrames.size();
        imageLabel->setPixmap(walkingDownFrames[currentDownFrameIndex]);
    }

    next = boundedPosition(next);
    if (next != m_absolutePos && !m_collisionDetector->isColliding(m_collisionArea.translated(next)))
        setInitialPosition(next);

    if (isIdle) {
        switch (m_lastDirection) {
        case PlayerDirection::Right:
            imageLabel->setPixmap(idleRightFrames[currentIdleFrameIndex]);
            break;
        case PlayerDirection::Left:
            imageLabel->setPixmap(idleLeftFrames[currentIdleFrameIndex]);
            break;
        case PlayerDirection::Up:
            imageLabel->setPixmap(idleUpFrames[currentIdleFrameIndex]);
            break;
        case PlayerDirection::Down:
            imageLabel->setPixmap(idleFrames[currentIdleFrameIndex]); // 使用原朝下动画
            break;
        }
        currentIdleFrameIndex = (currentIdleFrameIndex + 1) % idleFrames.size();
    }




}

void ImageMovementWidget::updateActionAnimation() {
    if (m_currentActionFrameIndex < m_currentActionFrames.size()) {
        imageLabel->setPixmap(m_currentActionFrames[m_currentActionFrameIndex]);
        m_currentActionFrameIndex++;
    } else {
        m_actionTimer->stop();
        m_isActing = false;
        // 恢复空闲状态动画
        updateAnimation();
    }
}



void ImageMovementWidget::handleToolAction() {
    if (m_inputBlocked) return;
    const ToolType toolType = m_controller->currentTool().type;
    if (!m_toolAnimations.contains(toolType)) return;

    m_isActing = true;
    const ToolAnimation& anim = m_toolAnimations[toolType];

    switch(m_lastDirection) {
    case PlayerDirection::Right: m_currentActionFrames = anim.rightFrames; break;
    case PlayerDirection::Left:  m_currentActionFrames = anim.leftFrames;  break;
    case PlayerDirection::Up:    m_currentActionFrames = anim.upFrames;    break;
    case PlayerDirection::Down:  m_currentActionFrames = anim.downFrames;  break;
    }

    if (!m_currentActionFrames.isEmpty()) {
        m_currentActionFrameIndex = 0;
        m_actionTimer->start(100); // 每100ms更新一帧
    } else {
        m_isActing = false;
    }
}

// rpg.cpp 中实现
void ImageMovementWidget::useCurrentToolAt(QPoint mapPos) {
    if (m_inputBlocked || !m_mapWidget || m_mapWidget->isInputBlocked() ||
        !m_mapWidget->isValidTile(m_mapWidget->mapPointToTile(mapPos)))
        return;
    handleToolAction();
    m_controller->useToolAt(mapPos);
}
