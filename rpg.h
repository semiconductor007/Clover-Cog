#include "collisiondetector.h"


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


private:
     CollisionDetector* m_collisionDetector = nullptr;
           QRect m_collisionArea; // 碰撞区域（相对于控件的位置）

protected:
    // 新增：重写 showEvent 确保控件获取焦点
    void showEvent(QShowEvent *event) override {
        setFocus();
        QWidget::showEvent(event);
    }


private:
    void initUI();
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void updateAnimation();

    QLabel *imageLabel;
    QPixmap spriteSheet;
    QPixmap idleSheet;
    QList<QPixmap> walkingRightFrames;
    QList<QPixmap> walkingLeftFrames;
    QList<QPixmap> walkingUpFrames;
    QList<QPixmap> walkingDownFrames;
    QList<QPixmap> idleFrames;
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
