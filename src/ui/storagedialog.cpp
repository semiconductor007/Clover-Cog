#include "storagedialog.h"
#include "farmicons.h"
#include "slotgrid.h"
#include <QScrollArea>

#include <QHeaderView>
#include <QTableWidget>

namespace {
constexpr int ItemColumn = 0;
constexpr int BagColumn = 1;
constexpr int ChestColumn = 2;
}

StorageDialog::StorageDialog(QWidget *parent)
    : FarmPanel(QStringLiteral("农场储物箱"), parent)
    , m_table(new QTableWidget(0, 3, this))
    , m_depositOne(makeButton(QStringLiteral("存入 1 份"), QStringLiteral("depositOne")))
    , m_depositAll(makeButton(QStringLiteral("全部存入"), QStringLiteral("depositAll")))
    , m_withdrawOne(makeButton(QStringLiteral("取出 1 份"), QStringLiteral("withdrawOne")))
    , m_withdrawAll(makeButton(QStringLiteral("全部取出"), QStringLiteral("withdrawAll"))) {
    setObjectName(QStringLiteral("storageDialog"));
    m_chestGrid=new SlotGrid(true,this); m_chestGrid->setObjectName(QStringLiteral("chestSlots"));
    m_bagGrid=new SlotGrid(false,this); m_bagGrid->setObjectName(QStringLiteral("storageBagSlots"));
    auto *scroll=new QScrollArea(this); scroll->setWidgetResizable(false);
    scroll->setWidget(m_chestGrid); scroll->setFixedSize(546,170);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    body->addWidget(scroll);
    body->addWidget(m_bagGrid);
    for (auto *grid : {m_chestGrid,m_bagGrid}) grid->moveRequested=[this](StackMove move){emit stackMoveRequested(move);};
    m_table->setObjectName(QStringLiteral("storageRows"));
    m_table->setAccessibleName(QStringLiteral("随身背包与储物箱物品数量"));
    m_table->setHorizontalHeaderLabels({QStringLiteral("物品"), QStringLiteral("随身背包"), QStringLiteral("储物箱")});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setIconSize(QSize(23, 24));
    m_table->verticalHeader()->hide();
    m_table->verticalHeader()->setDefaultSectionSize(28);
    m_table->horizontalHeader()->setSectionResizeMode(ItemColumn, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(BagColumn, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ChestColumn, QHeaderView::ResizeToContents);
    m_table->setStyleSheet(QStringLiteral(
        "QTableWidget { background: #fff4d8; color: #382f29; gridline-color: #d6c5a2; }"
        "QTableWidget::item:selected { background: #dde6bf; color: #24392a; }"
        "QHeaderView::section { background: #dfc99a; color: #3e3328; padding: 4px; }"));
    for (CropType crop : CropCatalog::cropTypes()) {
        const auto &item = CropCatalog::definition(crop);
        addRow(StorageItemKind::Harvest, crop, item.localizedName, FarmIcons::crop(item.harvestIcon));
    }
    for (CropType crop : CropCatalog::cropTypes()) {
        const auto &item = CropCatalog::definition(crop);
        addRow(StorageItemKind::Seed, crop, item.localizedName + QStringLiteral("种子"), FarmIcons::crop(item.seedIcon));
    }
    addRow(StorageItemKind::Fertilizer, CropType::None, QStringLiteral("基础肥料"), FarmIcons::fertilizer());
    m_table->hide();

    auto *actions = new QHBoxLayout;
    actions->addWidget(m_depositOne);
    actions->addWidget(m_depositAll);
    actions->addStretch();
    actions->addWidget(m_withdrawOne);
    actions->addWidget(m_withdrawAll);
    body->addLayout(actions);
    addFooter(QStringLiteral("选中物品后存取。只有随身农产品占用背包格数。"));
    const auto choose=[this](const InventoryStack &stack) {
        for (int i=0;i<m_rows.size();++i) {
            const auto &row=m_rows[i];
            const int item=row.kind==StorageItemKind::Fertilizer ? StackItem::Fertilizer :
                row.kind==StorageItemKind::Seed ? StackItem::seed(row.crop) : StackItem::harvest(row.crop);
            if (item==stack.item) {m_table->setCurrentCell(i,0);updateButtons();break;}
        }
    };
    m_bagGrid->slotChosen=[this,choose](int i){choose(m_bagGrid->cells()[i]);};
    m_chestGrid->slotChosen=[this,choose](int i){choose(m_chestGrid->cells()[i]);};
    connect(m_table, &QTableWidget::currentCellChanged, this, &StorageDialog::updateButtons);
    connect(m_depositOne, &QPushButton::clicked, this, [this] { requestTransfer(StorageDirection::Deposit, false); });
    connect(m_depositAll, &QPushButton::clicked, this, [this] { requestTransfer(StorageDirection::Deposit, true); });
    connect(m_withdrawOne, &QPushButton::clicked, this, [this] { requestTransfer(StorageDirection::Withdraw, false); });
    connect(m_withdrawAll, &QPushButton::clicked, this, [this] { requestTransfer(StorageDirection::Withdraw, true); });
    updateButtons();
    resize(590,680);
}

void StorageDialog::addRow(StorageItemKind kind, CropType crop, const QString &name, const QPixmap &icon) {
    const int row = m_table->rowCount();
    m_rows.append({kind, crop});
    m_table->insertRow(row);
    m_table->setItem(row, ItemColumn, new QTableWidgetItem(QIcon(slotIcon(icon)), name));
    for (int column : {BagColumn, ChestColumn}) {
        auto *count = new QTableWidgetItem;
        count->setTextAlignment(Qt::AlignCenter);
        count->setData(Qt::DisplayRole, 0);
        m_table->setItem(row, column, count);
    }
}

void StorageDialog::setState(const EconomyService &economy) {
    m_bagGrid->setState(economy.bagSlots(),economy.revision());
    m_chestGrid->setState(economy.chestSlots(),economy.revision());
    const auto &inventory = economy.inventory();
    const auto &storage = economy.storage();
    summary->setText(QStringLiteral("背包 %1 / %2").arg(inventory.usedCapacity()).arg(inventory.capacity()));
    int firstAvailable = -1;
    for (int index = 0; index < m_rows.size(); ++index) {
        const Row &row = m_rows.at(index);
        int carried = 0;
        int stored = 0;
        switch (row.kind) {
        case StorageItemKind::Harvest:
            carried = inventory.count(row.crop);
            stored = storage.harvestCounts.value(static_cast<int>(row.crop));
            break;
        case StorageItemKind::Seed:
            carried = economy.seedCount(row.crop);
            stored = storage.seedCounts.value(static_cast<int>(row.crop));
            break;
        case StorageItemKind::Fertilizer:
            carried = economy.fertilizerCount();
            stored = storage.fertilizerCount;
            break;
        }
        m_table->item(index, BagColumn)->setData(Qt::DisplayRole, carried);
        m_table->item(index, ChestColumn)->setData(Qt::DisplayRole, stored);
        if (firstAvailable < 0 && (carried || stored))
            firstAvailable = index;
    }
    if (m_table->currentRow() < 0)
        m_table->setCurrentCell(qMax(0, firstAvailable), ItemColumn);
    updateButtons();
}

void StorageDialog::updateButtons() {
    const int row = m_table->currentRow();
    const bool canDeposit = row >= 0 && m_table->item(row, BagColumn)->data(Qt::DisplayRole).toInt() > 0;
    const bool canWithdraw = row >= 0 && m_table->item(row, ChestColumn)->data(Qt::DisplayRole).toInt() > 0;
    m_depositOne->setEnabled(canDeposit);
    m_depositAll->setEnabled(canDeposit);
    m_withdrawOne->setEnabled(canWithdraw);
    m_withdrawAll->setEnabled(canWithdraw);
}

void StorageDialog::requestTransfer(StorageDirection direction, bool all) {
    const int index = m_table->currentRow();
    if (index < 0)
        return;
    const Row &row = m_rows.at(index);
    const int column = direction == StorageDirection::Deposit ? BagColumn : ChestColumn;
    const int available = m_table->item(index, column)->data(Qt::DisplayRole).toInt();
    if (available > 0)
        emit transferRequested(row.kind, row.crop, direction, all ? available : 1);
}
