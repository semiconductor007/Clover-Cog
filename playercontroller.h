#ifndef PLAYERCONTROLLER_H
#define PLAYERCONTROLLER_H

#include "tooltype.h"

#include <QObject>
#include <QPoint>
#include <QPointer>

class MapWidget;

enum class PlayerDirection {
    Right,
    Left,
    Up,
    Down
};

class PlayerController : public QObject {
    Q_OBJECT

public:
    explicit PlayerController(MapWidget *mapWidget, QObject *parent = nullptr);

    const ToolProperty &currentTool() const { return m_currentTool; }
    void setCurrentTool(int toolIndex);
    void useToolAt(const QPoint &mapPoint);
    void useAxeAt(const QPoint &playerCenter, PlayerDirection direction);

signals:
    void toolChanged(ToolType type);

private:
    QPointer<MapWidget> m_mapWidget;
    QVector<ToolProperty> m_tools;
    ToolProperty m_currentTool;
};

#endif // PLAYERCONTROLLER_H
