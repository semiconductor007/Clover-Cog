#include "rpg.h"
#include "mapwidget.h"
#include<QMouseEvent>

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

    // 初始化工具属性列表（索引0~5对应物品栏图片）
    m_tools = {
        {ToolType::EmptyHand,   "空手",    0,  ":/characte/Premium Charakter Spritesheet.png"},
        {ToolType::Axe,         "斧子",    30, ":/characte/Premium Charakter Spritesheet.png"},
        {ToolType::Hoe,         "锄头",    15, ":/characte/Premium Charakter Spritesheet.png"},
        {ToolType::WateringCan, "水壶",    5,  ":/characte/Premium Charakter Spritesheet.png"},
        {ToolType::Seed1,       "向日葵种子",0, ":/anim/planting.png"},
        {ToolType::Seed2,       "土豆种子", 0, ":/anim/planting.png"}
    };
    m_currentTool = m_tools[0]; // 初始化为空手



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

    setAttribute(Qt::WA_AcceptTouchEvents, false);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);


}

//切换工具
void ImageMovementWidget::setCurrentTool(int toolIndex) {
    if(toolIndex >= 0 && toolIndex < m_tools.size()) {
        m_currentTool = m_tools[toolIndex];
        qDebug() << "切换工具到:" << m_currentTool.name;

        // 更新角色动画
        QPixmap animPix(m_currentTool.animFile);
        if(!animPix.isNull()) {
            imageLabel->setPixmap(animPix.scaled(32,32));
        }
    }
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


    // 初始化斧子动画
    ToolAnimation axeAnim;
    for(int i=0; i<8; ++i){
        // 向右砍伐
        axeAnim.rightFrames.append(spriteSheet.copy(i*48, 18*48, 48, 48));
        // 向左砍伐
        axeAnim.leftFrames.append(axeAnim.rightFrames.last().transformed(QTransform().scale(-1, 1)));
        // 向上砍伐
        axeAnim.upFrames.append(spriteSheet.copy(i*48, 17*48, 48, 48));
        // 向下砍伐
        axeAnim.downFrames.append(spriteSheet.copy(i*48, 16*48, 48, 48));
    }
    m_toolAnimations[ToolType::Axe] = axeAnim;

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
    m_collisionArea = QRect(16, 16, 16, 16); // 中心16x16
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
         m_lastDirection = Direction::Right;

        break;
    case Qt::Key_A:
        isMovingLeft = true;
        isMovingRight = false;
        isMovingUp = false;
        isMovingDown = false;
        m_lastDirection = Direction::Left;

        break;
    case Qt::Key_W:
        isMovingUp = true;
        isMovingRight = false;
        isMovingLeft = false;
        isMovingDown = false;
        m_lastDirection = Direction::Up;

        break;
    case Qt::Key_S:
        isMovingDown = true;
        isMovingRight = false;
        isMovingLeft = false;
        isMovingUp = false;
         m_lastDirection = Direction::Down;

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
    if (m_isActing) return;
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

    if (map) {
        QPoint relativePos(
            m_absolutePos.x() - map->offsetX(),
            m_absolutePos.y() - map->offsetY()
            );
        this->move(relativePos);
        this->setGeometry(relativePos.x(), relativePos.y(), 48, 48); // 强制更新几何位置
    }
    // 空闲状态处理
    if (isIdle) {
        switch (m_lastDirection) {
        case Direction::Right:
            imageLabel->setPixmap(idleRightFrames[currentIdleFrameIndex]);
            break;
        case Direction::Left:
            imageLabel->setPixmap(idleLeftFrames[currentIdleFrameIndex]);
            break;
        case Direction::Up:
            imageLabel->setPixmap(idleUpFrames[currentIdleFrameIndex]);
            break;
        case Direction::Down:
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
    if (!m_toolAnimations.contains(m_currentTool.type)) return;

    m_isActing = true;
    const ToolAnimation& anim = m_toolAnimations[m_currentTool.type];

    switch(m_lastDirection) {
    case Direction::Right: m_currentActionFrames = anim.rightFrames; break;
    case Direction::Left:  m_currentActionFrames = anim.leftFrames;  break;
    case Direction::Up:    m_currentActionFrames = anim.upFrames;    break;
    case Direction::Down:  m_currentActionFrames = anim.downFrames;  break;
    }

    if (!m_currentActionFrames.isEmpty()) {
        m_currentActionFrameIndex = 0;
        m_actionTimer->start(100); // 每100ms更新一帧
    } else {
        m_isActing = false;
    }
}
