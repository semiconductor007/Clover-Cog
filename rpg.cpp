#include "rpg.h"
#include "mapwidget.h"


void ImageMovementWidget::setInitialPosition(const QPoint &pos) {
    position = pos;
    this->move(position);
    qDebug() << "[Position] Initialized to:" << position;
}


ImageMovementWidget::ImageMovementWidget(QWidget *parent) : QWidget(parent) {
      setFocusPolicy(Qt::StrongFocus);
    initUI();
    position = QPoint(0, 0);
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
    MapWidget* mapWidget = qobject_cast<MapWidget*>(parent);
    if (mapWidget) {
        qDebug() << "传递的图块尺寸:"
                 << mapWidget->tileWidth() << "x" << mapWidget->tileHeight(); // 调试输出
        m_collisionDetector = new CollisionDetector(
            mapWidget->collisionLayer(),
            mapWidget->tileWidth(),  // 必须 > 0
            mapWidget->tileHeight()
            );
    }


}

void ImageMovementWidget::initUI() {
    setGeometry(300, 300, 800, 600);
    setWindowTitle("Image Movement with Keyboard");

    // 加载精灵图（确保路径正确）
    spriteSheet.load(":/character/Walk.png");
    idleSheet.load(":/character/Idle.png");

    int frameWidth = 32;
    int frameHeight = 32;



    // 初始化空闲帧
    for (int i = 0; i < 4; ++i) {
        idleFrames.append(idleSheet.copy(i * frameWidth, 0, frameWidth, frameHeight));
    }

    // 初始化各方向动画帧
    walkingRightFrames.clear();
    for (int i = 0; i < 6; ++i) {
        walkingRightFrames.append(spriteSheet.copy(i * frameWidth, 2 * frameHeight, frameWidth, frameHeight));
    }

    // 水平翻转生成向左走的帧
    for (const QPixmap& rightFrame : walkingRightFrames) {
        QPixmap leftFrame = rightFrame.transformed(QTransform().scale(-1, 1));
        walkingLeftFrames.append(leftFrame);
    }

    // 其他方向帧
    for (int i = 0; i < 6; ++i) {
        walkingUpFrames.append(spriteSheet.copy(i * frameWidth, 1 * frameHeight, frameWidth, frameHeight));
        walkingDownFrames.append(spriteSheet.copy(i * frameWidth, 0 * frameHeight, frameWidth, frameHeight));
    }

    imageLabel = new QLabel(this);
    imageLabel->setPixmap(idleFrames.first());
    imageLabel->setAlignment(Qt::AlignCenter); // 居中显示
    this->setFixedSize(32, 32); // 固定控件尺寸为角色大小
    imageLabel->setFixedSize(32, 32); // 确保标签尺寸一致
m_collisionArea = QRect(12, 8, 8, 16); // 中心16x16
    // 设置透明背景
    imageLabel->setAttribute(Qt::WA_TranslucentBackground);
    imageLabel->setAutoFillBackground(false);
}

void ImageMovementWidget::keyPressEvent(QKeyEvent *event) {
    // 忽略自动重复的按键事件
    if (event->isAutoRepeat()) {
        event->ignore();
        return;
    }

    isIdle = false;
    switch (event->key()) {
    case Qt::Key_D:
        isMovingRight = true;
        isMovingLeft = false;
        isMovingUp = false;
        isMovingDown = false;
        break;
    case Qt::Key_A:
        isMovingLeft = true;
        isMovingRight = false;
        isMovingUp = false;
        isMovingDown = false;
        break;
    case Qt::Key_W:
        isMovingUp = true;
        isMovingRight = false;
        isMovingLeft = false;
        isMovingDown = false;
        break;
    case Qt::Key_S:
        isMovingDown = true;
        isMovingRight = false;
        isMovingLeft = false;
        isMovingUp = false;
        break;
    default:
        QWidget::keyPressEvent(event);
        return;
    }
    event->accept();
}

void ImageMovementWidget::keyReleaseEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) {
        event->ignore();
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
    if (!m_collisionDetector) {
        qDebug() << "Collision detector not initialized!";
        return;
    }

    const int step = 10;

    QWidget *mapWidget = qobject_cast<QWidget*>(parent());
    if (!mapWidget || mapWidget->width() <= 0) return;

    QPoint newPos = position; // 正确继承当前坐标

    // 各方向移动处理（使用else if确保单一方向）
    if (isMovingRight) {
        newPos.setX(position.x() + step);
        currentRightFrameIndex = (currentRightFrameIndex + 1) % walkingRightFrames.size();
        imageLabel->setPixmap(walkingRightFrames[currentRightFrameIndex]);
    }
    if (isMovingLeft) {  // 改为独立if判断，允许斜向移动
        newPos.setX(position.x() - step);
        currentLeftFrameIndex = (currentLeftFrameIndex + 1) % walkingLeftFrames.size();
        imageLabel->setPixmap(walkingLeftFrames[currentLeftFrameIndex]);
    }
    if (isMovingUp) {
        newPos.setY(position.y() - step);
        currentUpFrameIndex = (currentUpFrameIndex + 1) % walkingUpFrames.size();
        imageLabel->setPixmap(walkingUpFrames[currentUpFrameIndex]);
    }
    if (isMovingDown) {
        newPos.setY(position.y() + step);
        currentDownFrameIndex = (currentDownFrameIndex + 1) % walkingDownFrames.size();
        imageLabel->setPixmap(walkingDownFrames[currentDownFrameIndex]);
    }



    QPoint newAbsolutePos = m_absolutePos; // 使用绝对坐标

    if (isMovingRight) newAbsolutePos.rx() += step;
    if (isMovingLeft) newAbsolutePos.rx() -= step;
    if (isMovingUp) newAbsolutePos.ry() -= step;
    if (isMovingDown) newAbsolutePos.ry() += step;

    // 碰撞检测（使用绝对坐标）
    QRect newCollisionRect(
        newAbsolutePos.x() + m_collisionArea.x(),
        newAbsolutePos.y() + m_collisionArea.y(),
        m_collisionArea.width(),
        m_collisionArea.height()
        );
    if (m_collisionDetector->isColliding(newCollisionRect)) {
        return;
    }

    m_absolutePos = newAbsolutePos;

    // 获取父控件 MapWidget
    MapWidget* map = qobject_cast<MapWidget*>(parent());
    if (map) {
        // 计算角色中心点
        int centerX = m_absolutePos.x() + width() / 2;
        int centerY = m_absolutePos.y() + height() / 2;

        // 更新视口中心
        map->setViewportCenter(centerX, centerY);

        // 计算角色在视口中的相对位置并移动
        QPoint relativePos(
            m_absolutePos.x() - map->offsetX(),
            m_absolutePos.y() - map->offsetY()
            );
        this->move(relativePos);
    }
    // 通知地图控件更新视口
    // 空闲状态处理
    if (isIdle) {
        currentIdleFrameIndex = (currentIdleFrameIndex + 1) % idleFrames.size();
        imageLabel->setPixmap(idleFrames[currentIdleFrameIndex]);
    }





}
