#ifndef SEEDSHOPDIALOG_H
#define SEEDSHOPDIALOG_H

#include "economy/economyservice.h"
#include "farmpanel.h"

class SeedShopDialog : public FarmPanel {
    Q_OBJECT
public:
    explicit SeedShopDialog(QWidget *parent = nullptr);
    void setState(const EconomyService &economy);

signals:
    void purchaseRequested(CropType type, int quantity);
    void fertilizerPurchaseRequested(int quantity);

private:
    QHash<CropType, QLabel *> m_seedCounts;
    QLabel *m_fertilizerCount = nullptr;
};

#endif
