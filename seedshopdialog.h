#ifndef SEEDSHOPDIALOG_H
#define SEEDSHOPDIALOG_H

#include "cropdefinition.h"

#include <QDialog>

class QLabel;

class SeedShopDialog : public QDialog {
    Q_OBJECT

public:
    explicit SeedShopDialog(QWidget *parent = nullptr);

    void setState(int coins, int wheatSeeds, int potatoSeeds);
    void setMessage(const QString &message);

signals:
    void purchaseRequested(CropType type, int quantity);

private:
    QLabel *m_coinsLabel = nullptr;
    QLabel *m_wheatLabel = nullptr;
    QLabel *m_potatoLabel = nullptr;
    QLabel *m_messageLabel = nullptr;
};

#endif // SEEDSHOPDIALOG_H
