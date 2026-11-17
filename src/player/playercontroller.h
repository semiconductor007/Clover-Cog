#ifndef PLAYERCONTROLLER_H
#define PLAYERCONTROLLER_H

#include "tooltype.h"

#include <QObject>
#include <QPoint>
#include <QPointer>
#include <functional>
#include <utility>

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
    void setTilePositionGetter(std::function<QPoint()> getter) { m_tilePosition = std::move(getter); }
    bool canReachTile(const QPoint &tile) const;
    QPoint playerTilePosition() const {return m_tilePosition?m_tilePosition():QPoint(-1,-1);}

signals:
    void toolChanged(ToolType type);

private:
    QPointer<MapWidget> m_mapWidget;
    ToolProperty m_currentTool;
    std::function<QPoint()> m_tilePosition;
};

#endif // PLAYERCONTROLLER_H
