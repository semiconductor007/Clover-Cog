#ifndef RPG_H
#define RPG_H

#include "collisiondetector.h"
#include "playercontroller.h"
#include "tooltype.h"

#include <QKeyEvent>
#include <QLabel>
#include <QList>
#include <QMap>
#include <QPoint>
#include <QPointer>
#include <QPixmap>
#include <QRect>
#include <QTimer>
#include <QVector>
#include <QWidget>
#include <memory>

class MapWidget;



class ImageMovementWidget : public QWidget {
    Q_OBJECT
private:
     std::unique_ptr<CollisionDetector> m_collisionDetector;
     QPointer<MapWidget> m_mapWidget;
     PlayerController *m_controller = nullptr;
     QRect m_collisionArea; // 碰撞区域（相对于控件的位置）
     QPoint m_absolutePos; // 角色在地图中的绝对坐标（像素）
public:
    explicit ImageMovementWidget(MapWidget *mapWidget, QWidget *parent = nullptr);
    void setInitialPosition(const QPoint &pos); // 新增方法
    void handleToolAction();
    // 新增：获取当前工具属性
    const ToolProperty &getCurrentTool() const { return m_controller->currentTool(); }



protected:
    // 新增：重写 showEvent 确保控件获取焦点
    void showEvent(QShowEvent *event) override {
        setFocus();
        QWidget::showEvent(event);
    }
    void mousePressEvent(QMouseEvent* event) override;


public slots:
    void setCurrentTool(int toolIndex); // 根据物品栏索引切换工具
    void useCurrentToolAt(QPoint mapPos);
private://工具动画
    QMap<ToolType, ToolAnimation> m_toolAnimations;
    bool m_isActing = false; // 是否正在执行动作

    void updateActionAnimation();

    QTimer* m_actionTimer;
    QVector<QPixmap> m_currentActionFrames;
    int m_currentActionFrameIndex = 0;



    void initUI();
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void updateAnimation();

    PlayerDirection m_lastDirection = PlayerDirection::Down;
    QLabel *imageLabel;
    QPixmap spriteSheet;
    QPixmap idleSheet;
    QList<QPixmap> walkingRightFrames;
    QList<QPixmap> walkingLeftFrames;
    QList<QPixmap> walkingUpFrames;
    QList<QPixmap> walkingDownFrames;
    QList<QPixmap> idleFrames;
    QList<QPixmap> idleRightFrames;
    QList<QPixmap> idleLeftFrames;
    QList<QPixmap> idleUpFrames;
    int currentIdleFrameIndex;
    int currentRightFrameIndex;
    int currentLeftFrameIndex;
    int currentUpFrameIndex;
    int currentDownFrameIndex;
    QPoint position;
    bool isMovingRight;
    bool isMovingLeft;
    bool isMovingUp;
    bool isMovingDown;
    bool isIdle;
    QTimer *animationTimer;

};

#endif // RPG_H
