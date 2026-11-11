#include "marketdialog.h"
#include "farmicons.h"

MarketDialog::MarketDialog(QWidget *parent)
    : FarmPanel(QStringLiteral("农产品收购点"), parent) {
    setObjectName(QStringLiteral("marketDialog"));
    auto *goods = new QGridLayout;
    goods->setColumnStretch(1, 1);
    goods->setHorizontalSpacing(16);
    tableHeading(goods, 0, QStringLiteral("农产品"), 2);
    tableHeading(goods, 2, QStringLiteral("单价"));
    tableHeading(goods, 3, QStringLiteral("数量"));
    tableHeading(goods, 4, QStringLiteral("小计"));
    int row = 1;
    for (CropType crop : CropCatalog::cropTypes()) {
        const auto &def = CropCatalog::definition(crop);
        productCell(goods, row, def.localizedName, FarmIcons::crop(def.harvestIcon));
        goods->addWidget(new QLabel(QStringLiteral("%1 金币").arg(def.sellPrice), this), row, 2);
        Line line{new QLabel(this), new QLabel(this)};
        line.count->setObjectName(QStringLiteral("quantity_%1").arg(def.id));
        line.subtotal->setObjectName(QStringLiteral("subtotal_%1").arg(def.id));
        goods->addWidget(line.count, row, 3);
        goods->addWidget(line.subtotal, row, 4);
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
    addFooter(QStringLiteral("按收购单价结算背包中的全部农产品。"), m_sell);
}

void MarketDialog::setState(const EconomyService &economy) {
    summary->setText(QStringLiteral("金币  %1").arg(economy.coins()));
    qint64 total = 0;
    for (auto it = m_lines.cbegin(); it != m_lines.cend(); ++it) {
        const int count = economy.inventory().count(it.key());
        const qint64 subtotal = qint64(count) * CropCatalog::definition(it.key()).sellPrice;
        it.value().count->setText(QString::number(count));
        it.value().subtotal->setText(QStringLiteral("%1 金币").arg(subtotal));
        total += subtotal;
    }
    m_total->setText(QStringLiteral("共 %1 份    合计 %2 金币")
                         .arg(economy.inventory().usedCapacity()).arg(total));
    m_sell->setEnabled(total > 0);
}
