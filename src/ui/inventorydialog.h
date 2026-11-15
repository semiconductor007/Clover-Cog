#ifndef INVENTORYDIALOG_H
#define INVENTORYDIALOG_H
#include "economy/economyservice.h"
#include "farmpanel.h"
class SlotGrid;
class InventoryDialog : public FarmPanel {
    Q_OBJECT
public:
    explicit InventoryDialog(QWidget *parent=nullptr);
    void setState(const EconomyService &economy,ToolType selected,int gameDay=0,int active=-2);
signals:
    void toolRequested(ToolType tool);
    void slotRequested(int slot);
    void stackMoveRequested(StackMove move);
protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *object,QEvent *event) override;
private:
    SlotGrid *m_grid;
    QLabel *m_date;
};
#endif
