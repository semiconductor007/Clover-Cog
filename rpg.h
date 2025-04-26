#include "collisiondetector.h"
#include "tooltype.h"


#ifndef RPG_H
#define RPG_H

#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QKeyEvent>
#include <QTimer>
#include<QPainter>
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QXmlStreamReader>
#include<QString>



class ImageMovementWidget : public QWidget {
    Q_OBJECT
public:
    ImageMovementWidget(QWidget *parent = nullptr);
    void setInitialPosition(const QPoint &pos); // 新增方法
    void handleToolAction();
    // 新增：获取当前工具属性
    const ToolProperty &getCurrentTool() const { return m_currentTool; }



private:
     CollisionDetector* m_collisionDetector = nullptr;
     QRect m_collisionArea; // 碰撞区域（相对于控件的位置）
     QPoint m_absolutePos; // 角色在地图中的绝对坐标（像素）

protected:
    // 新增：重写 showEvent 确保控件获取焦点
    void showEvent(QShowEvent *event) override {
        setFocus();
        QWidget::showEvent(event);
    }


public slots:
    void setCurrentTool(int toolIndex); // 根据物品栏索引切换工具

private://工具动画
    ToolProperty m_currentTool; // 当前装备的工具属性
    QVector<ToolProperty> m_tools; // 工具属性列表
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

    enum class Direction { Right, Left, Up, Down };
    Direction m_lastDirection = Direction::Down;
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

    QList<QList<int>> mapLayers;
    int tileWidth;
    int tileHeight;
};

#endif // RPG_H
