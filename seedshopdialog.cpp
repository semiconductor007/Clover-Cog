#include "seedshopdialog.h"

#include "cropdefinition.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QWidget *createSeedRow(CropType type, QLabel **countLabel,
                       SeedShopDialog *dialog) {
    QWidget *row = new QWidget(dialog);
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);

    const CropDefinition &definition = CropCatalog::definition(type);
    QLabel *name = new QLabel(
        QStringLiteral("%1  %2 COINS").arg(definition.displayName)
            .arg(definition.seedPrice), row);
    *countLabel = new QLabel(row);
    QPushButton *buyOne = new QPushButton(QStringLiteral("BUY 1"), row);
    QPushButton *buyFive = new QPushButton(QStringLiteral("BUY 5"), row);

    layout->addWidget(name, 1);
    layout->addWidget(*countLabel);
    layout->addWidget(buyOne);
    layout->addWidget(buyFive);

    QObject::connect(buyOne, &QPushButton::clicked, dialog,
                     [dialog, type]() { emit dialog->purchaseRequested(type, 1); });
    QObject::connect(buyFive, &QPushButton::clicked, dialog,
                     [dialog, type]() { emit dialog->purchaseRequested(type, 5); });
    return row;
}

} // namespace

SeedShopDialog::SeedShopDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("SEED SHOP"));
    setModal(true);
    setMinimumWidth(390);

    QVBoxLayout *layout = new QVBoxLayout(this);
    m_coinsLabel = new QLabel(this);
    layout->addWidget(m_coinsLabel);
    layout->addWidget(createSeedRow(CropType::Wheat, &m_wheatLabel, this));
    layout->addWidget(createSeedRow(CropType::Potato, &m_potatoLabel, this));

    m_messageLabel = new QLabel(QStringLiteral("SELECT SEEDS TO BUY"), this);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_messageLabel);

    QPushButton *closeButton = new QPushButton(QStringLiteral("CLOSE"), this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeButton);

    setStyleSheet(QStringLiteral(
        "QDialog { background: #4c301e; color: white; }"
        "QLabel { color: white; padding: 4px; }"
        "QPushButton { background: #e8c98f; color: #4c301e;"
        " border: 2px solid #8f6038; padding: 5px 10px; }"
        "QPushButton:pressed { background: #c99f67; }"));
}

void SeedShopDialog::setState(int coins, int wheatSeeds, int potatoSeeds) {
    m_coinsLabel->setText(QStringLiteral("COINS: %1").arg(coins));
    m_wheatLabel->setText(QStringLiteral("OWNED: %1").arg(wheatSeeds));
    m_potatoLabel->setText(QStringLiteral("OWNED: %1").arg(potatoSeeds));
}

void SeedShopDialog::setMessage(const QString &message) {
    m_messageLabel->setText(message);
}
