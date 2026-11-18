#include "seedshopdialog.h"
#include "farmicons.h"

SeedShopDialog::SeedShopDialog(QWidget *parent)
    : FarmPanel(QStringLiteral("种子商店"), parent) {
    setObjectName(QStringLiteral("shopDialog"));
    auto *goods = new QGridLayout;
    goods->setColumnStretch(1, 1);
    goods->setHorizontalSpacing(10);
    goods->setVerticalSpacing(14);
    tableHeading(goods, 0, QStringLiteral("商品"), 2);
    tableHeading(goods, 2, QStringLiteral("单价"));
    tableHeading(goods, 3, QStringLiteral("持有"));
    tableHeading(goods, 4, QStringLiteral("购买数量"), 2);

    int row = 1;
    auto addGoods = [&](const QString &id, const QString &name, const QPixmap &icon,
                        int price, auto requestPurchase) {
        const int item = id == QStringLiteral("fertilizer") ? StackItem::Fertilizer
            : StackItem::seed(CropCatalog::fromId(id));
        productCell(goods, row, name, icon, FarmIcons::itemTooltip(item));
        goods->addWidget(new QLabel(QStringLiteral("%1 金币").arg(price), this), row, 2);
        auto *count = new QLabel(this);
        count->setObjectName(QStringLiteral("owned_%1").arg(id));
        goods->addWidget(count, row, 3);
        int column = 4;
        for (int quantity : {1, 5}) {
            auto *button = makeButton(QStringLiteral("买 %1").arg(quantity),
                                      QStringLiteral("buy_%1_%2").arg(id).arg(quantity));
            button->setAccessibleName(QStringLiteral("购买 %1 份%2").arg(quantity).arg(name));
            button->setToolTip(QStringLiteral("花费 %1 金币").arg(price * quantity));
            goods->addWidget(button, row, column++);
            connect(button, &QPushButton::clicked, this,
                    [requestPurchase, quantity] { requestPurchase(quantity); });
        }
        ++row;
        return count;
    };
    for (CropType crop : CropCatalog::cropTypes()) {
        const auto &def = CropCatalog::definition(crop);
        m_seedCounts[crop] = addGoods(def.id, def.localizedName + QStringLiteral("种子"),
                                      FarmIcons::crop(def.seedIcon), def.seedPrice,
                                      [this, crop](int n) { emit purchaseRequested(crop, n); });
    }
    m_fertilizerCount = addGoods(QStringLiteral("fertilizer"), QStringLiteral("基础肥料"),
                                 FarmIcons::fertilizer(), EconomyService::FertilizerPrice,
                                 [this](int n) { emit fertilizerPurchaseRequested(n); });
    body->addLayout(goods);
    auto *upgrades = new QHBoxLayout;
    m_upgradeCan = makeButton(QString(), QStringLiteral("upgradeCan"));
    m_upgradeAxe = makeButton(QString(), QStringLiteral("upgradeAxe"));
    m_upgradeCan->setToolTip(FarmIcons::itemTooltip(StackItem::Water));
    m_upgradeAxe->setToolTip(FarmIcons::itemTooltip(StackItem::Axe));
    connect(m_upgradeCan, &QPushButton::clicked, this, &SeedShopDialog::canUpgradeRequested);
    connect(m_upgradeAxe, &QPushButton::clicked, this, &SeedShopDialog::axeUpgradeRequested);
    upgrades->addWidget(m_upgradeCan);
    upgrades->addWidget(m_upgradeAxe);
    body->addLayout(upgrades);
    addFooter(QStringLiteral("购入的种子和肥料可在背包中选用。"), heading);
    useTradeStyle();
}

void SeedShopDialog::setState(const EconomyService &economy) {
    summary->setText(QStringLiteral("金币  %1").arg(economy.coins()));
    for (auto it = m_seedCounts.cbegin(); it != m_seedCounts.cend(); ++it)
        it.value()->setText(QString::number(economy.seedCount(it.key())));
    m_fertilizerCount->setText(QString::number(economy.fertilizerCount()));
    const auto showUpgrade = [&](QPushButton *button, const QString &name, int price, bool owned) {
        button->setText(owned ? QStringLiteral("%1 · 已升级").arg(name)
                              : QStringLiteral("%1 · %2 金币").arg(name).arg(price));
        button->setEnabled(!owned && economy.coins() >= price);
    };
    showUpgrade(m_upgradeCan, QStringLiteral("水壶扩容"), EconomyService::CanUpgradePrice, economy.canUpgraded());
    showUpgrade(m_upgradeAxe, QStringLiteral("强化斧头"), EconomyService::AxeUpgradePrice, economy.axeUpgraded());
}
