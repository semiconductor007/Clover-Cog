#ifndef INVENTORYPANEL_H
#define INVENTORYPANEL_H
#include "economyservice.h"
#include <QWidget>
#include <QPixmap>

// Display snapshot only; the panel never owns or changes real inventory.
class InventoryPanel : public QWidget {
    Q_OBJECT
public:
    explicit InventoryPanel(QWidget *parent = nullptr);
    void setState(const EconomyState &state);
signals:
    void closeRequested();
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    EconomyState m_snapshot;
    QPixmap m_slots;
};
#endif
