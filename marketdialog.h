#ifndef MARKETDIALOG_H
#define MARKETDIALOG_H

#include "economyservice.h"
#include "farmpanel.h"

class MarketDialog : public FarmPanel {
    Q_OBJECT
public:
    explicit MarketDialog(QWidget *parent = nullptr);
    void setState(const EconomyService &economy);

signals:
    void saleRequested();

private:
    struct Line {
        QLabel *count;
        QLabel *subtotal;
    };
    QHash<CropType, Line> m_lines;
    QLabel *m_total = nullptr;
    QPushButton *m_sell = nullptr;
};

#endif
