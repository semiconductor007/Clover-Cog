#include "inventorydialog.h"
#include "farmicons.h"

#include <QListWidget>
#include <QTabWidget>

namespace {
QListWidgetItem *addEntry(QListWidget *list, const QPixmap &pixmap, ToolType tool) {
    QIcon icon(pixmap);
    icon.addPixmap(pixmap, QIcon::Selected);
    auto *item = new QListWidgetItem(icon, QString(), list);
    item->setData(Qt::UserRole, static_cast<int>(tool));
    return item;
}

void updateEntry(QListWidgetItem *item, const QString &name, int count, bool equipped) {
    QString text = QStringLiteral("%1    × %2").arg(name).arg(count);
    if (equipped)
        text += QStringLiteral("    · 已选用");
    item->setText(text);
    item->setHidden(count == 0);
}

void updateEmptyState(QListWidget *list) {
    // The final entry is the empty-category message.
    QListWidgetItem *firstVisible = nullptr;
    for (int row = 0; row < list->count() - 1 && !firstVisible; ++row) {
        if (!list->item(row)->isHidden())
            firstVisible = list->item(row);
    }
    list->item(list->count() - 1)->setHidden(firstVisible != nullptr);
    if (!list->currentItem() || list->currentItem()->isHidden())
        list->setCurrentItem(firstVisible);
}
} // namespace

InventoryDialog::InventoryDialog(QWidget *parent)
    : FarmPanel(QStringLiteral("背包"), parent), m_tabs(new QTabWidget(this)),
      m_date(new QLabel(this)) {
    setObjectName(QStringLiteral("inventoryDialog"));
    m_date->setObjectName(QStringLiteral("bagDate"));
    body->addWidget(m_date);
    m_tabs->setObjectName(QStringLiteral("bagCategories"));
    m_tabs->setAccessibleName(QStringLiteral("物品分类"));
    m_harvest = addCategory(QStringLiteral("农产品"), QStringLiteral("bagHarvest"));
    m_seeds = addCategory(QStringLiteral("种子"), QStringLiteral("bagSeeds"));
    m_fertilizer = addCategory(QStringLiteral("肥料"), QStringLiteral("bagFertilizer"));

    for (CropType crop : CropCatalog::cropTypes()) {
        const auto &def = CropCatalog::definition(crop);
        addEntry(m_harvest, slotIcon(FarmIcons::crop(def.harvestIcon)), ToolType::EmptyHand);
        addEntry(m_seeds, slotIcon(FarmIcons::crop(def.seedIcon)), toolForSeed(crop));
    }
    addEntry(m_fertilizer, slotIcon(FarmIcons::fertilizer()), ToolType::Fertilizer);
    auto addEmpty = [](QListWidget *list, const QString &message) {
        auto *empty = new QListWidgetItem(message, list);
        empty->setFlags(Qt::ItemIsEnabled);
    };
    addEmpty(m_harvest, QStringLiteral("还没有农产品。成熟后收获即可入包。"));
    addEmpty(m_seeds, QStringLiteral("种子用完了，可以去种子商店购买。"));
    addEmpty(m_fertilizer, QStringLiteral("还没有肥料，可以去种子商店购买。"));

    body->addWidget(m_tabs);
    addFooter(QStringLiteral("农产品可带到收购点出售。"));
    connect(m_tabs, &QTabWidget::currentChanged, this, [this] {
        if (m_tabs->currentWidget() == m_harvest)
            setMessage(QStringLiteral("农产品可带到收购点出售。"));
        else if (m_tabs->currentWidget() == m_seeds)
            setMessage(QStringLiteral("选用后，点击适季的已耕地播种。"));
        else
            setMessage(QStringLiteral("选用后，点击未成熟作物施肥。每株限一次。"));
        m_tabs->currentWidget()->setFocus();
    });
    auto *keys = new QLabel(QStringLiteral("← → 分类   ↑ ↓ 浏览   回车选用   Tab / Esc 关闭"), this);
    body->insertWidget(body->count() - 1, keys);
    setMinimumWidth(sizeHint().width());
    m_harvest->setFocus();
}

QListWidget *InventoryDialog::addCategory(const QString &title, const QString &objectName) {
    auto *list = new QListWidget(m_tabs);
    list->setObjectName(objectName);
    list->setAccessibleName(title);
    list->setIconSize(QSize(32, 34));
    list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    list->setMinimumHeight(176);
    connect(list, &QListWidget::itemClicked, this, &InventoryDialog::choose);
    m_tabs->addTab(list, title);
    return list;
}

void InventoryDialog::setState(const EconomyService &economy, ToolType selected, int gameDay) {
    const SeasonDate date = SeasonCalendar::dateForDay(gameDay);
    m_date->setText(QStringLiteral("第 %1 年 · %2 · 第 %3 天")
                        .arg(date.year).arg(SeasonCalendar::seasonName(date.season))
                        .arg(date.dayOfSeason));
    const auto &inventory = economy.inventory();
    summary->setText(QStringLiteral("农产品  %1 / %2").arg(inventory.usedCapacity())
                         .arg(inventory.capacity()));
    int row = 0;
    for (CropType crop : CropCatalog::cropTypes()) {
        const QString &name = CropCatalog::definition(crop).localizedName;
        updateEntry(m_harvest->item(row), name, inventory.count(crop), false);
        auto *seed = m_seeds->item(row);
        const bool equipped = selected == toolForSeed(crop);
        updateEntry(seed, name + QStringLiteral("种子"), economy.seedCount(crop), equipped);
        if (m_firstRefresh && equipped && !seed->isHidden()) {
            m_tabs->setCurrentWidget(m_seeds);
            m_seeds->setCurrentItem(seed);
        }
        ++row;
    }
    const bool fertilizerSelected = selected == ToolType::Fertilizer;
    updateEntry(m_fertilizer->item(0), QStringLiteral("基础肥料"),
                economy.fertilizerCount(), fertilizerSelected);
    if (m_firstRefresh && fertilizerSelected && economy.fertilizerCount() > 0)
        m_tabs->setCurrentWidget(m_fertilizer);
    for (QListWidget *list : {m_harvest, m_seeds, m_fertilizer})
        updateEmptyState(list);
    m_firstRefresh = false;
}

void InventoryDialog::choose(QListWidgetItem *item) {
    if (!isVisible() || !item || item->isHidden())
        return;
    const auto tool = static_cast<ToolType>(item->data(Qt::UserRole).toInt());
    if (tool == ToolType::EmptyHand)
        return;
    emit toolRequested(tool);
    accept();
}

bool InventoryDialog::eventFilter(QObject *object, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Tab && key->modifiers() == Qt::NoModifier) {
            if (!key->isAutoRepeat())
                reject();
            return true;
        }
        if (key->key() == Qt::Key_Left || key->key() == Qt::Key_Right) {
            int step = key->key() == Qt::Key_Left ? -1 : 1;
            m_tabs->setCurrentIndex((m_tabs->currentIndex() + step + m_tabs->count()) % m_tabs->count());
            return true;
        }
        if (object == m_tabs->currentWidget() &&
            (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter ||
             key->key() == Qt::Key_Space)) {
            if (!key->isAutoRepeat())
                choose(static_cast<QListWidget *>(object)->currentItem());
            return true;
        }
    }
    return FarmPanel::eventFilter(object, event);
}
