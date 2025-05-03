// tooltype.h
#ifndef TOOLTYPE_H
#define TOOLTYPE_H

#include <QString>
#include <QPixmap>

enum class ToolType {
    EmptyHand,  // 空手
    Axe,        // 斧子
    Hoe,        // 锄头
    WateringCan,// 水壶
    Wheat,      // 种子1
    Seed2       // 种子2
};

struct ToolAnimation {
    QVector<QPixmap> leftFrames;  // 向左动画帧
    QVector<QPixmap> rightFrames; // 向右动画帧
    QVector<QPixmap> upFrames;    // 向上动画帧
    QVector<QPixmap> downFrames;  // 向下动画帧
};

struct ToolProperty {
    ToolType type;
    QString name;
    int damage;      // 工具伤害值
    QString animFile;// 对应动画文件路径
};

#endif // TOOLTYPE_H
