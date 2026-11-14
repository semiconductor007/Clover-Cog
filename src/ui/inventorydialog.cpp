#include "inventorydialog.h"
#include "farmicons.h"

#include <QListWidget>
#include <QTabWidget>
#include <QStyledItemDelegate>
#include <QTabBar>

namespace {
class HotbarRow : public QWidget {
public:
    using QWidget::QWidget;
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.drawPixmap(rect(), QPixmap(QStringLiteral(":/farm-ui/hotbar.png")));
    }
};

class HotbarButton : public QPushButton {
public:
    using QPushButton::QPushButton;
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        const bool hasItem = !property("bagCount").isValid() || property("bagCount").toInt() > 0;
        if (hasItem) p.drawPixmap(QRect(18, 12, 36, 36), icon().pixmap(32, 32));
        if (isChecked() || hasFocus()) {
            p.setPen(QPen(QColor("#a04f32"), 3));
            p.drawRect(rect().adjusted(4, 2, -5, -12));
        }
        if (hasItem && property("bagCount").isValid()) {
            QFont f = font(); f.setBold(true); p.setFont(f);
            p.setPen(QColor("#513521"));
            p.drawText(rect().adjusted(4, 0, -8, -16), Qt::AlignRight | Qt::AlignBottom,
                       property("bagCount").toString());
        }
    }
};

void pixelPanel(QPainter &painter, const QRect &rect) {
    const QPixmap source = QPixmap(QStringLiteral(":/farm-ui/slots.png")).copy(8, 8, 32, 34);
    const int edge = 12;
    const int xs[] = {rect.left(), rect.left() + edge, rect.right() + 1 - edge, rect.right() + 1};
    const int ys[] = {rect.top(), rect.top() + edge, rect.bottom() + 1 - edge, rect.bottom() + 1};
    const int sx[] = {0, 6, 26, 32}, sy[] = {0, 6, 28, 34};
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x)
            painter.drawPixmap(QRect(xs[x], ys[y], xs[x+1]-xs[x], ys[y+1]-ys[y]), source,
                QRect(sx[x], sy[y], sx[x+1]-sx[x], sy[y+1]-sy[y]));
}

class SlotDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override { return QSize(72,72); }
    void paint(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        if (!index.data(Qt::UserRole).isValid()) return;
        const QRect slot = option.rect.adjusted(3, 3, -3, -3);
        p->save();
        pixelPanel(*p, slot);
        if (option.state & QStyle::State_Selected) {
            p->setPen(QPen(QColor("#8e4934"), 3));
            p->drawRect(slot.adjusted(2, 2, -2, -2));
        }
        const QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        p->drawPixmap(QRect(slot.center().x()-18, slot.center().y()-20, 36, 36), icon.pixmap(32,32));
        const QString count = index.data(Qt::UserRole + 2).toString();
        QFont font = option.font;
        font.setBold(true);
        font.setPixelSize(13);
        p->setFont(font);
        const QRect number = slot.adjusted(3, 0, -5, -4);
        p->setPen(QColor("#fff8df"));
        p->drawText(number.translated(1,1), Qt::AlignRight | Qt::AlignBottom, count);
        p->setPen(QColor("#513521"));
        p->drawText(number, Qt::AlignRight | Qt::AlignBottom, count);
        p->restore();
    }
};

class InventoryGrid : public QListWidget {
public:
    using QListWidget::QListWidget;
protected:
    void paintEvent(QPaintEvent *event) override {
        QListWidget::paintEvent(event);
        QPainter painter(viewport());
        // Empty presentation slots complete the grid; they do not create inventory.
        const int columns = qMax(1, viewport()->width() / gridSize().width());
        for (int row = 0; row < 2; ++row)
            for (int col = 0; col < columns; ++col) {
                const QRect cell(col*gridSize().width(), row*gridSize().height(), gridSize().width(), gridSize().height());
                const QModelIndex index = indexAt(cell.center());
                if (!index.isValid() || !index.data(Qt::UserRole).isValid())
                    pixelPanel(painter, cell.adjusted(3,3,-3,-3));
            }
    }
};

QListWidgetItem *addEntry(QListWidget *list, const QPixmap &pixmap, ToolType tool) {
    QIcon icon(pixmap);
    icon.addPixmap(pixmap, QIcon::Selected);
    auto *item = new QListWidgetItem(icon, QString(), list);
    item->setData(Qt::UserRole, static_cast<int>(tool));
    return item;
}

void updateEntry(QListWidgetItem *item, const QString &name, int count, bool equipped) {
    QString text = QStringLiteral("%1\n× %2").arg(name).arg(count);
    if (equipped)
        text += QStringLiteral(" ✓");
    item->setText(text);
    item->setData(Qt::UserRole + 2, count);
    const QString description = item->data(Qt::UserRole + 1).toString();
    item->setToolTip(QStringLiteral("%1 · 持有 %2%3\n%4").arg(name).arg(count)
        .arg(equipped ? QStringLiteral(" · 已选用") : QString()).arg(description));
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
      m_date(new QLabel(this)), m_details(new QLabel(this)) {
    setObjectName(QStringLiteral("inventoryDialog"));
    m_date->setObjectName(QStringLiteral("bagDate"));
    body->addWidget(m_date);
    auto *barRow = new HotbarRow(this);
    barRow->setObjectName(QStringLiteral("bagHotbarRow"));
    barRow->setFixedSize(522, 144);
    auto *tools = new QHBoxLayout(barRow);
    tools->setContentsMargins(12, 42, 6, 12);
    tools->setSpacing(0);
    const QPixmap toolSheet(QStringLiteral(":/tilesets/sprout/Basic_tools_and_meterials.png"));
    for (ToolType tool : PlayableTools) {
        auto *button = new HotbarButton(barRow);
        button->setObjectName(QStringLiteral("bagTool_%1").arg(static_cast<int>(tool)));
        button->setAutoDefault(false);
        button->setAccessibleName(toolName(tool));
        button->setFixedSize(72, 90);
        QPixmap icon = tool == ToolType::EmptyHand
            ? QPixmap(QStringLiteral(":/ui/sprout/Catpaw Mouse icon.png"))
            : toolSheet.copy(tool == ToolType::Hoe ? 32 : 0, 0, 16, 16);
        if (seedCropForTool(tool) != CropType::None)
            icon = FarmIcons::crop(CropCatalog::definition(seedCropForTool(tool)).seedIcon);
        if (tool == ToolType::Fertilizer) icon = FarmIcons::fertilizer();
        button->setIcon(QIcon(icon));
        button->setIconSize(QSize(32, 34));
        button->setCheckable(true);
        button->setProperty("tool", static_cast<int>(tool));
        button->setToolTip(QStringLiteral("选用%1，返回农场后使用；不会消耗物品。").arg(toolName(tool)));
        connect(button, &QPushButton::clicked, this, [this, tool] {
            if (!isVisible()) return;
            emit toolRequested(tool);
            accept();
        });
        tools->addWidget(button);
        m_tools.append(button);
    }
    m_tabs->setObjectName(QStringLiteral("bagCategories"));
    m_tabs->setAccessibleName(QStringLiteral("物品分类"));
    m_harvest = addCategory(QStringLiteral("农产品"), QStringLiteral("bagHarvest"));
    m_seeds = addCategory(QStringLiteral("种子"), QStringLiteral("bagSeeds"));
    m_fertilizer = addCategory(QStringLiteral("肥料"), QStringLiteral("bagFertilizer"));

    for (CropType crop : CropCatalog::cropTypes()) {
        const auto &def = CropCatalog::definition(crop);
        addEntry(m_harvest, FarmIcons::crop(def.harvestIcon), ToolType::EmptyHand)
            ->setData(Qt::UserRole + 1, QStringLiteral("农产品，占用随身容量。带到收购点出售，或存入箱子。"));
        addEntry(m_seeds, FarmIcons::crop(def.seedIcon), toolForSeed(crop))
            ->setData(Qt::UserRole + 1, QStringLiteral("选用后点击适季的已耕地播种。需要 %1 个浇水生长日。")
                .arg(def.growthDays));
    }
    addEntry(m_fertilizer, FarmIcons::fertilizer(), ToolType::Fertilizer)
        ->setData(Qt::UserRole + 1, QStringLiteral("未成熟作物每株限一次，下个浇水生长日额外成长一天。"));
    auto addEmpty = [](QListWidget *list, const QString &message) {
        auto *empty = new QListWidgetItem(message, list);
        empty->setFlags(Qt::ItemIsEnabled);
    };
    addEmpty(m_harvest, QStringLiteral("还没有农产品。成熟后收获即可入包。"));
    addEmpty(m_seeds, QStringLiteral("种子用完了，可以去种子商店购买。"));
    addEmpty(m_fertilizer, QStringLiteral("还没有肥料，可以去种子商店购买。"));

    m_tabs->tabBar()->hide();
    m_tabs->setFixedHeight(156);
    body->addWidget(barRow, 0, Qt::AlignHCenter);
    body->addWidget(m_tabs, 0, Qt::AlignHCenter);
    m_details->setObjectName(QStringLiteral("bagDetails"));
    m_details->setWordWrap(true);
    m_details->setMinimumHeight(40);
    body->addWidget(m_details);
    auto *keys = new QLabel(QStringLiteral("首排对应实际物品栏 · 0 切换空手\nCtrl+←/→ 浏览分类 · E / Tab / Esc 关闭"), this);
    keys->setWordWrap(true);
    body->addWidget(keys);
    addFooter(QStringLiteral("农产品可带到收购点出售。"));
    connect(m_tabs, &QTabWidget::currentChanged, this, [this] {
        if (m_tabs->currentWidget() == m_harvest)
            setMessage(QStringLiteral("农产品可带到收购点出售。"));
        else if (m_tabs->currentWidget() == m_seeds)
            setMessage(QStringLiteral("选用后，点击适季的已耕地播种。"));
        else
            setMessage(QStringLiteral("选用后，点击未成熟作物施肥。每株限一次。"));
        m_tabs->currentWidget()->setFocus();
        auto *list = static_cast<QListWidget *>(m_tabs->currentWidget());
        m_details->setText(list->currentItem() ? list->currentItem()->toolTip() : QString());
    });
    body->setContentsMargins(28, 24, 28, 24);
    setStyleSheet(styleSheet() + QStringLiteral(
        "QDialog#inventoryDialog { background: transparent; }"
        "QListWidget { background: #c79b69; border: 3px solid #9b7048; }"
        "QListWidget::item { padding: 0; background: transparent; }"
        "QListWidget::item:selected { background: transparent; }"
        "QLabel#panelTitle { color: #68402a; font-size: 23px; }"
        "QLabel#panelSummary { background: transparent; color: #68402a; }"
        "QLabel#bagDetails { background: #f1d4a4; border: 2px solid #b68b5b; padding: 10px; }"
        "QPushButton:checked { background: #d9b37d; border-color: #8e4934; }"
        "QTabBar::tab { padding: 7px 18px; background: #d4ac79; border: 2px solid #9b7048; }"
        "QTabBar::tab:selected { background: #f1d4a4; color: #633f27; }"));
    for (QLabel *label : findChildren<QLabel *>()) label->hide();
    auto *close = findChild<QPushButton *>(QStringLiteral("closePanel"));
    close->setText(QStringLiteral("×"));
    close->setAccessibleName(QStringLiteral("关闭背包"));
    close->setToolTip(QStringLiteral("关闭背包（E / Tab / Esc）"));
    close->setFixedSize(28, 28);
    if (auto *buttons = body->itemAt(body->count() - 1)->layout())
        buttons->removeWidget(close);
    close->raise();
    body->setContentsMargins(12, 12, 12, 12);
    body->setSpacing(0);
    setMinimumWidth(546);
    resize(546, sizeHint().height());
    m_harvest->setFocus();
}

QListWidget *InventoryDialog::addCategory(const QString &title, const QString &objectName) {
    auto *list = new InventoryGrid(m_tabs);
    list->setItemDelegate(new SlotDelegate(list));
    list->setObjectName(objectName);
    list->setAccessibleName(title);
    list->setViewMode(QListView::IconMode);
    list->setMovement(QListView::Static);
    list->setResizeMode(QListView::Adjust);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    list->setIconSize(QSize(48, 51));
    list->setGridSize(QSize(72, 72));
    list->setSpacing(0);
    list->setWordWrap(true);
    list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    list->setFixedSize(522, 152);
    m_tabs->setFixedWidth(522);
    connect(list, &QListWidget::itemClicked, this, &InventoryDialog::choose);
    connect(list, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *item) {
        m_details->setText(item ? item->toolTip() : QString());
    });
    m_tabs->addTab(list, title);
    return list;
}

void InventoryDialog::setState(const EconomyService &economy, ToolType selected, int gameDay) {
    const SeasonDate date = SeasonCalendar::dateForDay(gameDay);
    m_date->setText(QStringLiteral("第 %1 年 · %2 · 第 %3 天")
                        .arg(date.year).arg(SeasonCalendar::seasonName(date.season))
                        .arg(date.dayOfSeason));
    const auto &inventory = economy.inventory();
    setProperty("selectedTool", static_cast<int>(selected));
    summary->setText(QStringLiteral("金币 %1 · 农产品 %2 / %3").arg(economy.coins())
                         .arg(inventory.usedCapacity()).arg(inventory.capacity()));
    for (auto *button : m_tools) {
        button->setChecked(button->property("tool").toInt() == static_cast<int>(selected));
        const ToolType tool = static_cast<ToolType>(button->property("tool").toInt());
        const CropType crop = seedCropForTool(tool);
        if (crop != CropType::None) button->setProperty("bagCount", economy.seedCount(crop));
        if (tool == ToolType::Fertilizer) button->setProperty("bagCount", economy.fertilizerCount());
        if (button->property("bagCount").isValid())
            button->setEnabled(button->property("bagCount").toInt() > 0);
        button->update();
    }
    int row = 0;
    for (CropType crop : CropCatalog::cropTypes()) {
        const QString &name = CropCatalog::definition(crop).localizedName;
        updateEntry(m_harvest->item(row), name, inventory.count(crop), false);
        auto *seed = m_seeds->item(row);
        const bool equipped = selected == toolForSeed(crop);
        updateEntry(seed, name + QStringLiteral("种子"), economy.seedCount(crop), equipped);
        if (m_firstRefresh && equipped && !seed->isHidden()) {
            m_seeds->setCurrentItem(seed);
        }
        ++row;
    }
    const bool fertilizerSelected = selected == ToolType::Fertilizer;
    updateEntry(m_fertilizer->item(0), QStringLiteral("基础肥料"),
                economy.fertilizerCount(), fertilizerSelected);
    for (QListWidget *list : {m_harvest, m_seeds, m_fertilizer})
        updateEmptyState(list);
    auto *list = static_cast<QListWidget *>(m_tabs->currentWidget());
    m_details->setText(list->currentItem() ? list->currentItem()->toolTip()
        : QStringLiteral("首排与角色物品栏同步。选择物品返回农场使用，查看不会消耗物品。"));
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

void InventoryDialog::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    pixelPanel(painter, rect().adjusted(2, 2, -2, -2));
}

void InventoryDialog::resizeEvent(QResizeEvent *event) {
    FarmPanel::resizeEvent(event);
    if (auto *close = findChild<QPushButton *>(QStringLiteral("closePanel"))) {
        close->move(width() - close->width() - 12, 12);
        close->raise();
    }
}

bool InventoryDialog::eventFilter(QObject *object, QEvent *event) {
    if (event->type() == QEvent::ToolTip || event->type() == QEvent::FocusIn) {
        if (auto *button = qobject_cast<QPushButton *>(object)) {
            if (button->objectName().startsWith(QStringLiteral("bagTool_")))
                m_details->setText(button->toolTip());
        }
    }
    if (event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        if ((key->key() == Qt::Key_Tab || key->key() == Qt::Key_E) && key->modifiers() == Qt::NoModifier) {
            if (!key->isAutoRepeat())
                reject();
            return true;
        }
        if ((key->key() == Qt::Key_Left || key->key() == Qt::Key_Right) && key->modifiers() == Qt::ControlModifier) {
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
