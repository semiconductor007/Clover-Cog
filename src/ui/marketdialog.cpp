#include "marketdialog.h"
#include "farmicons.h"

#include <QSpinBox>

MarketDialog::MarketDialog(QWidget *parent)
    : FarmPanel(QStringLiteral("农产品收购点"), parent) {
    setObjectName(QStringLiteral("marketDialog"));
    auto *goods = new QGridLayout;
    goods->setColumnStretch(1, 1);
    goods->setHorizontalSpacing(16);
    tableHeading(goods, 0, QStringLiteral("农产品"), 2);
    tableHeading(goods, 2, QStringLiteral("单价"));
    tableHeading(goods, 3, QStringLiteral("持有"));
    tableHeading(goods, 4, QStringLiteral("卖几份"));
    tableHeading(goods, 5, QStringLiteral("本次收入"));
    int row = 1;
    for (CropType crop : CropCatalog::cropTypes()) {
        const auto &def = CropCatalog::definition(crop);
        productCell(goods, row, def.localizedName, FarmIcons::crop(def.harvestIcon));
        goods->addWidget(new QLabel(QStringLiteral("%1 金币").arg(def.sellPrice), this), row, 2);
        auto *quantity = new QSpinBox(this);
        quantity->setObjectName(QStringLiteral("sellQuantity_%1").arg(def.id));
        quantity->setAccessibleName(QStringLiteral("%1出售数量").arg(def.localizedName));
        auto *sell = makeButton(QString(), QStringLiteral("sell_%1").arg(def.id));
        sell->setAccessibleName(QStringLiteral("出售%1").arg(def.localizedName));
        const auto updateQuote = [sell, price = def.sellPrice](int count) {
            sell->setText(QStringLiteral("卖出 · %1 金币").arg(qint64(count) * price));
        };
        connect(quantity, &QSpinBox::valueChanged, this, updateQuote);
        updateQuote(quantity->value());
        connect(sell, &QPushButton::clicked, this, [this, crop, quantity] {
            emit cropSaleRequested(crop, quantity->value());
        });
        Line line{new QLabel(this), quantity, sell};
        line.count->setObjectName(QStringLiteral("quantity_%1").arg(def.id));
        goods->addWidget(line.count, row, 3);
        goods->addWidget(quantity, row, 4);
        goods->addWidget(sell, row, 5);
        m_lines.insert(crop, line);
        ++row;
    }
    body->addLayout(goods);
    m_total = new QLabel(this);
    m_total->setObjectName(QStringLiteral("saleTotal"));
    m_total->setAlignment(Qt::AlignRight);
    body->addWidget(m_total);
    m_sell = makeButton(QStringLiteral("出售全部农产品"), QStringLiteral("sellAll"));
    connect(m_sell, &QPushButton::clicked, this, &MarketDialog::saleRequested);
    addFooter(QStringLiteral("输入或调整要卖的份数，再点同行的「卖出」。"), m_sell);
}

void MarketDialog::setState(const EconomyService &economy) {
    summary->setText(QStringLiteral("金币  %1").arg(economy.coins()));
    qint64 total = 0;
    for (auto it = m_lines.cbegin(); it != m_lines.cend(); ++it) {
        const int count = economy.inventory().count(it.key());
        const qint64 subtotal = qint64(count) * CropCatalog::definition(it.key()).sellPrice;
        it.value().count->setText(QString::number(count));
        it.value().quantity->setRange(count > 0 ? 1 : 0, count);
        it.value().quantity->setEnabled(count > 0);
        it.value().sell->setEnabled(count > 0);
        total += subtotal;
    }
    m_total->setText(QStringLiteral("背包共 %1 份    合计 %2 金币")
                         .arg(economy.inventory().usedCapacity()).arg(total));
    m_sell->setEnabled(total > 0);
}
