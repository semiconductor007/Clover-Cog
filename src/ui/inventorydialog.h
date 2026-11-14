#ifndef INVENTORYDIALOG_H
#define INVENTORYDIALOG_H

#include "economy/economyservice.h"
#include "farmpanel.h"
#include "player/tooltype.h"

class QListWidget;
class QListWidgetItem;
class QTabWidget;

class InventoryDialog : public FarmPanel {
    Q_OBJECT
public:
    explicit InventoryDialog(QWidget *parent = nullptr);
    void setState(const EconomyService &economy, ToolType selected, int gameDay = 0);

signals:
    void toolRequested(ToolType tool);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    QListWidget *addCategory(const QString &title, const QString &objectName);
    void choose(QListWidgetItem *item);
    QTabWidget *m_tabs;
    QListWidget *m_harvest;
    QListWidget *m_seeds;
    QListWidget *m_fertilizer;
    QLabel *m_date;
    QLabel *m_details;
    QList<QPushButton *> m_tools;
    bool m_firstRefresh = true;
};

#endif
