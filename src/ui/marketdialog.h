#ifndef MARKETDIALOG_H
#define MARKETDIALOG_H

#include "economy/economyservice.h"
#include "farmpanel.h"

class QSpinBox;

class MarketDialog : public FarmPanel {
    Q_OBJECT
public:
    explicit MarketDialog(QWidget *parent = nullptr);
    void setState(const EconomyService &economy);

signals:
    void cropSaleRequested(CropType crop, int quantity);
    void saleRequested();

private:
    struct Line {
        QLabel *count;
        QSpinBox *quantity;
        QPushButton *sell;
    };
    QHash<CropType, Line> m_lines;
    QLabel *m_total = nullptr;
    QPushButton *m_sell = nullptr;
};

#endif
