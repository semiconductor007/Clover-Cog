#include "mainwindow.h"
#include "gameconstants.h"
#include "inventorydialog.h"
#include "marketdialog.h"
#include "scenetransition.h"
#include "seedshopdialog.h"

#include <QAudioOutput>
#include <QCloseEvent>
#include <QFileInfo>
#include <QFrame>
#include <QGraphicsProxyWidget>
#include <QHBoxLayout>
#include <QMediaPlayer>
#include <QMouseEvent>
#include <QScreen>
#include <QSoundEffect>
#include <QTimer>
#include <QUrl>
#include <QWheelEvent>
#include <QtMath>
#include <utility>

namespace {
constexpr int CanvasWidth = 576;
constexpr int HeaderHeight = 34;
constexpr int MapHeight = 320;
constexpr int FooterHeight = 68;
constexpr int CanvasHeight = HeaderHeight + MapHeight + FooterHeight;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_gameClock(GameConstants::RealMillisecondsPerGameMinute)
    , m_economy(GameConstants::InitialInventoryCapacity)
    , m_gameCanvas(new QWidget)
    , m_gameScene(new QGraphicsScene(this))
    , m_gameView(new QGraphicsView(m_gameScene, this))
    , mapWidget(new MapWidget(m_gameCanvas))
    , customCursor(QPixmap(":/ui/sprout/Catpaw Mouse icon.png"), 8, 8)
    , clickCursor(QPixmap(":/ui/sprout/Catpaw holding Mouse icon.png"), 8, 8)
{
    setWindowTitle(QStringLiteral("四季农场"));
    setCursor(customCursor);
    m_gameClock.setElapsedGameMinutes(m_committedMinutes);

    m_gameCanvas->setFixedSize(CanvasWidth, CanvasHeight);
    QPalette canvasPalette = m_gameCanvas->palette();
    canvasPalette.setColor(QPalette::Window, QColor("#efdfb9"));
    m_gameCanvas->setPalette(canvasPalette);
    m_gameCanvas->setAutoFillBackground(true);
    m_gameScene->setSceneRect(m_gameCanvas->rect());
    m_gameView->setFrameShape(QFrame::NoFrame);
    m_gameView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gameView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gameView->setAlignment(Qt::AlignCenter);
    m_gameView->setBackgroundBrush(QColor("#26372e"));
    m_gameView->setRenderHint(QPainter::SmoothPixmapTransform, false);
    m_gameView->setTransformationAnchor(QGraphicsView::NoAnchor);
    m_gameView->setResizeAnchor(QGraphicsView::NoAnchor);
    setCentralWidget(m_gameView);
    m_gameView->viewport()->installEventFilter(this);

    // Reserve separate strips for the HUD and toolbar so both map edges stay playable.
    mapWidget->setGeometry(0, HeaderHeight, CanvasWidth, MapHeight);
    if (!mapWidget->loadMap(QStringLiteral(":/map/map6.tmj"))) {
        qCritical() << "Unable to initialize the game map";
    }
    mapWidget->setMarketArea(m_marketArea);
    mapWidget->setShopArea(m_shopArea);
    mapWidget->setBedArea(m_bedArea);
    m_seasons.load(mapWidget->originalTilesets());
    mapWidget->setTimeReference([this] { return m_committedMinutes; });

    auto *header = new QWidget(m_gameCanvas);
    header->setObjectName(QStringLiteral("farmHeader"));
    header->setGeometry(0, 0, CanvasWidth, HeaderHeight);
    header->setStyleSheet(QStringLiteral(
        "QWidget#farmHeader { background: #35533f; }"
        "QLabel { color: #fff4d8; font-size: 13px; }"
        "QLabel#gameTime { color: #ffe19c; font-weight: bold; }"));
    auto *headerRow = new QHBoxLayout(header);
    headerRow->setContentsMargins(12, 0, 12, 0);
    headerRow->setSpacing(14);
    dayLabel = new QLabel(header);
    dayLabel->setObjectName(QStringLiteral("gameDate"));
    timeLabel = new QLabel(header);
    timeLabel->setObjectName(QStringLiteral("gameTime"));
    m_coinsLabel = new QLabel(header);
    m_inventoryLabel = new QLabel(header);
    headerRow->addWidget(dayLabel);
    headerRow->addWidget(timeLabel);
    headerRow->addStretch();
    headerRow->addWidget(m_coinsLabel);
    headerRow->addWidget(m_inventoryLabel);

    character = new ImageMovementWidget(mapWidget, mapWidget);
    character->setInitialPosition(QPoint(400, 192));
    m_daylight = new DayNightOverlay(mapWidget);
    itemBar = new ItemBar(m_gameCanvas);
    const int footerTop = HeaderHeight + MapHeight;
    const QSize barSize = itemBar->sizeHint();
    itemBar->setGeometry(8, footerTop + (FooterHeight - barSize.height()) / 2,
                         barSize.width(), barSize.height());
    mapWidget->installEventFilter(this);
    const int textLeft = itemBar->geometry().right() + 16;
    const int textWidth = CanvasWidth - textLeft - 10;
    m_suppliesLabel = new QLabel(m_gameCanvas);
    m_suppliesLabel->setGeometry(textLeft, footerTop + 5, textWidth, 22);
    m_suppliesLabel->setStyleSheet(QStringLiteral("color: #35533f; font-size: 12px; font-weight: bold;"));
    m_statusLabel = new QLabel(m_gameCanvas);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setGeometry(textLeft, footerTop + 26, textWidth, FooterHeight - 29);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #453728; font-size: 12px;"));

    m_transition = new SceneTransition(m_gameCanvas);
    connect(m_transition, &SceneTransition::blackReached, this, &MainWindow::presentTime);
    connect(m_transition, &SceneTransition::finished, this, [this] {
        m_transitionActive = false;
        presentTime();
        refreshGameplayInput();
        if (m_panelOpen && m_panelFocusBeforeFade)
            m_panelFocusBeforeFade->setFocus(Qt::OtherFocusReason);
        m_panelFocusBeforeFade.clear();
    });

    auto *music = new QMediaPlayer(this);
    auto *output = new QAudioOutput(music);
    output->setVolume(0.28f);
    music->setAudioOutput(output);
    music->setLoops(QMediaPlayer::Infinite);
    music->setSource(QUrl(QStringLiteral("qrc:/sounds/wav/On_the_Farm.wav")));
    music->play();
    const auto sound = [this](const QString &name, const QString &url) {
        auto *effect = new QSoundEffect(this);
        effect->setObjectName(name);
        effect->setVolume(0.55f);
        effect->setSource(QUrl(url));
        return effect;
    };
    auto *water = sound(QStringLiteral("wateringSound"), QStringLiteral("qrc:/sounds/water.wav"));
    auto *harvest = sound(QStringLiteral("harvestSound"), QStringLiteral("qrc:/sounds/harvest.wav"));
    connect(mapWidget, &MapWidget::tileWatered, water, &QSoundEffect::play);
    connect(mapWidget, &MapWidget::cropHarvested, harvest, &QSoundEffect::play);
    connect(itemBar, &ItemBar::itemSelected, this, &MainWindow::updateHud);
    connect(itemBar, &ItemBar::itemSelected,
            character, &ImageMovementWidget::setCurrentTool);

    connect(mapWidget, &MapWidget::mapClicked,
            character, &ImageMovementWidget::useCurrentToolAt);

    connect(character, &ImageMovementWidget::inventoryRequested, this, &MainWindow::openInventory);
    mapWidget->setHarvestHandler([this](CropType type, int quantity) {
        return m_economy.collectHarvest(type, quantity);
    });
    mapWidget->setPlantHandler([this](CropType type, const std::function<bool()> &action) {
        return m_economy.plantWithSeed(type, action);
    });
    mapWidget->setFertilizeHandler([this](const std::function<bool()> &action) {
        return m_economy.fertilizeWithItem(action);
    });

    connect(mapWidget, &MapWidget::farmingFeedback, this, &MainWindow::showStatus);
    connect(mapWidget, &MapWidget::cropHarvested, this, [this](CropType type, int quantity) {
        showStatus(QStringLiteral("收获%1 × %2").arg(CropCatalog::definition(type).localizedName)
                                          .arg(quantity));
    });
    connect(mapWidget, &MapWidget::harvestBlocked, this, [this](CropType, int requiredCapacity) {
        showStatus(QStringLiteral("背包装不下，本次收获需要 %1 份空位").arg(requiredCapacity));
    });
    connect(mapWidget, &MapWidget::plantingOutOfSeason, this, [this](CropType type) {
        showStatus(QStringLiteral("本季不能播种%1").arg(CropCatalog::definition(type).localizedName));
    });
    connect(mapWidget, &MapWidget::plantingBlocked, this, [this](CropType type) {
        showStatus(QStringLiteral("%1种子不足，可到种子商店购买")
                       .arg(CropCatalog::definition(type).localizedName));
    });

    connect(character, &ImageMovementWidget::interactionRequested, this, [this](QPoint playerTile) {
        if (m_panelOpen || m_transitionActive) {
            return;
        }
        if (m_shopArea.contains(playerTile)) {
            openSeedShop();
        } else if (m_marketArea.contains(playerTile)) {
            openMarket();
        } else if (m_bedArea.contains(playerTile)) {
            sleepUntilMorning();
        } else {
            showStatus(QStringLiteral("站在商店、收购点或床边按 E"));
        }
    });
    // MapWidget emits this only after both the world action and the inventory action commit.
    connect(mapWidget, &MapWidget::worldChanged, this, [this] {
        updateHud();
        saveGame();
    });

    loadGame();
    applyClock(ClockEvent::Load);
    updateHud();

    auto *timeUpdateTimer = new QTimer(this);
    connect(timeUpdateTimer, &QTimer::timeout, this, &MainWindow::updateTime);
    timeUpdateTimer->start(100);
    auto *autosaveTimer = new QTimer(this);
    connect(autosaveTimer, &QTimer::timeout, this, [this] { applyClock(ClockEvent::Save); });
    autosaveTimer->start(60000);

    m_gameScene->addWidget(m_gameCanvas)->setCacheMode(QGraphicsItem::NoCache);
    const QSize available = screen()->availableGeometry().size();
    const bool fitsDouble = available.width() >= CanvasWidth * 2 &&
                            available.height() >= CanvasHeight * 2 + 36;
    resize(m_gameCanvas->size() * (fitsDouble ? 2 : 1));
    updateGameScale();
}

void MainWindow::updateTime() {
    applyClock(ClockEvent::Tick);
}

void MainWindow::applyClock(ClockEvent event) {
    const qint64 minutes = m_gameClock.elapsedGameMinutes();
    if (event == ClockEvent::Tick && minutes == m_committedMinutes) return;
    const int day = int(minutes / (24 * 60));
    const bool crossedDay = m_lastProcessedDay < day;
    const bool crossedSeason = day / SeasonCalendar::DaysPerSeason !=
                               currentGameDay() / SeasonCalendar::DaysPerSeason;
    const bool woke = event == ClockEvent::WakeUp;
    if (event != ClockEvent::Load && !m_transitionActive && (woke || crossedSeason)) {
        // Freeze the outgoing farm before changing any crop, HUD or seasonal texture.
        m_transitionActive = m_transition->start();
        if (m_panelOpen) m_panelFocusBeforeFade = m_panelOpen->focusWidget();
        refreshGameplayInput();
    }

    m_committedMinutes = minutes;
    const DayAdvanceResult result = processDayTransitions();
    if (woke) {
        showStatus(crossedDay
            ? QStringLiteral("早上好！作物成长 %1 株，缺水 %2 株。")
                  .arg(result.cropsAdvanced).arg(result.cropsStalled)
            : QStringLiteral("天亮了，田里的浇水状态已保留。"));
    } else if (crossedDay && event != ClockEvent::Load) {
        showStatus(QStringLiteral("新的一天，作物成长 %1 株，缺水 %2 株。")
                       .arg(result.cropsAdvanced).arg(result.cropsStalled));
    }
    if (!m_transitionActive) presentTime();
    if (crossedDay || woke || event == ClockEvent::Save)
        saveGame(event != ClockEvent::Save);
}

DayAdvanceResult MainWindow::processDayTransitions() {
    DayAdvanceResult total;
    while (m_lastProcessedDay < currentGameDay()) {
        const DayAdvanceResult result = mapWidget->advanceDay(m_lastProcessedDay);
        total.cropsAdvanced += result.cropsAdvanced;
        total.cropsStalled += result.cropsStalled;
        total.soilRecovered += result.soilRecovered;
        total.fertilizerBonusesUsed += result.fertilizerBonusesUsed;
        ++m_lastProcessedDay;
    }
    return total;
}

void MainWindow::presentTime() {
    const int day = currentGameDay();
    if (m_visibleDay != day) {
        const SeasonDate date = SeasonCalendar::dateForDay(day);
        const Season season = date.season;
        if (m_visibleDay < 0 || season != SeasonCalendar::dateForDay(m_visibleDay).season) {
            mapWidget->setSeasonAppearance(m_seasons.tilesets(season),
                [this, season](QPainter &painter, const QPoint &offset, const WorldState &world) {
                    m_seasons.paintDecorations(season, painter, offset, world);
                });
        }
        dayLabel->setText(QStringLiteral("第 %1 年 · %2 %3 日")
                              .arg(date.year).arg(SeasonCalendar::seasonName(season))
                              .arg(date.dayOfSeason));
        m_visibleDay = day;
        refreshOpenPanel();
    }
    timeLabel->setText(QStringLiteral("%1:%2").arg(currentGameHour(), 2, 10, QLatin1Char('0'))
                                              .arg(currentGameMinute(), 2, 10, QLatin1Char('0')));
    m_daylight->setGameMinutes(m_committedMinutes);
}

void MainWindow::sleepUntilMorning() {
    if (m_transitionActive || m_panelOpen || !m_bedArea.contains(character->currentTilePosition()))
        return;
    if (!m_gameClock.sleepToNextMorning()) {
        showStatus(QStringLiteral("可在 20:00 至次日 06:00 前到床边睡觉。"));
        return;
    }
    applyClock(ClockEvent::WakeUp);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_gameView->viewport() && event->type() == QEvent::Resize) {
        updateGameScale();
    }
    if (m_transitionActive && m_panelOpen &&
        (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease ||
         event->type() == QEvent::ShortcutOverride)) {
        auto *widget = qobject_cast<QWidget *>(watched);
        if (widget && widget->window() == m_panelOpen) {
            event->accept();
            return true;
        }
    }
    // The toolbar sits outside the scene, but the wheel still works over the farm.
    if (watched == mapWidget && event->type() == QEvent::Wheel) {
        itemBar->wheelEvent(static_cast<QWheelEvent *>(event));
        return true;
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::updateGameScale() {
    if (!m_gameView || !m_gameCanvas) {
        return;
    }

    const QSize viewportSize = m_gameView->viewport()->size();
    if (viewportSize.isEmpty()) {
        return;
    }

    const qreal availableScale = qMin(
        static_cast<qreal>(viewportSize.width()) / m_gameCanvas->width(),
        static_cast<qreal>(viewportSize.height()) / m_gameCanvas->height());
    const qreal dpr = m_gameView->viewport()->devicePixelRatioF();
    const qreal pixelScale = availableScale >= 1.0
        ? qFloor(availableScale * dpr) / dpr : availableScale;

    m_gameView->resetTransform();
    m_gameView->scale(pixelScale, pixelScale);
    m_gameView->centerOn(m_gameScene->sceneRect().center());
}

void MainWindow::updateHud() {
    const Inventory &inventory = m_economy.inventory();
    m_coinsLabel->setText(QStringLiteral("金币 %1").arg(m_economy.coins()));
    m_inventoryLabel->setText(QStringLiteral("背包 %1 / %2")
                                  .arg(inventory.usedCapacity()).arg(inventory.capacity()));
    const CropType crop = seedCropForTool(itemBar->selectedTool());
    if (crop != CropType::None) {
        m_suppliesLabel->setText(QStringLiteral("%1种子 %2 份  ·  肥料 %3 份")
            .arg(CropCatalog::definition(crop).localizedName)
            .arg(m_economy.seedCount(crop)).arg(m_economy.fertilizerCount()));
    } else {
        qint64 seeds = 0;
        for (CropType type : CropCatalog::cropTypes()) seeds += m_economy.seedCount(type);
        m_suppliesLabel->setText(QStringLiteral("种子 %1 份  ·  肥料 %2 份")
                                     .arg(seeds).arg(m_economy.fertilizerCount()));
    }
}

void MainWindow::showStatus(const QString &message) {
    m_statusLabel->setText(message);
}

void MainWindow::openInventory() {
    if (m_panelOpen || m_transitionActive) {
        return;
    }
    auto *dialog = new InventoryDialog(this);
    connect(dialog, &InventoryDialog::toolRequested, this, [this](ToolType tool) {
        itemBar->selectTool(tool);
    });
    showFarmPanel(dialog);
}

void MainWindow::openSeedShop() {
    auto *dialog = new SeedShopDialog(this);
    const auto purchase = [this, dialog](ToolType tool, int quantity) {
        if (!m_shopArea.contains(character->currentTilePosition())) {
            dialog->setMessage(QStringLiteral("请到种子商店购买"));
            return;
        }
        const CropType crop = seedCropForTool(tool);
        const bool fertilizer = tool == ToolType::Fertilizer;
        const auto result = fertilizer ? m_economy.buyFertilizer(quantity)
                                       : m_economy.buySeeds(crop, quantity);
        if (!result.success) {
            dialog->setMessage(result.failure == SeedPurchaseFailure::InsufficientCoins
                                   ? QStringLiteral("金币不足")
                                   : QStringLiteral("无法购买该数量，库存未变"));
            return;
        }
        const QString item = fertilizer ? QStringLiteral("基础肥料")
                                        : CropCatalog::definition(crop).localizedName + QStringLiteral("种子");
        finishTrade(dialog, QStringLiteral("已购入%1 × %2，花费 %3 金币")
                                .arg(item).arg(quantity).arg(result.coinsSpent));
    };
    connect(dialog, &SeedShopDialog::purchaseRequested, this, [purchase](CropType crop, int quantity) {
        purchase(toolForSeed(crop), quantity);
    });
    connect(dialog, &SeedShopDialog::fertilizerPurchaseRequested, this, [purchase](int quantity) {
        purchase(ToolType::Fertilizer, quantity);
    });
    showFarmPanel(dialog);
}

void MainWindow::openMarket() {
    auto *dialog = new MarketDialog(this);
    connect(dialog, &MarketDialog::saleRequested, this, [this, dialog] {
        const SaleResult sale = m_economy.sellAll(character->currentTilePosition(), m_marketArea);
        if (!sale.success) {
            dialog->setMessage(QStringLiteral("未能出售，请检查站点、库存和金币余额"));
            return;
        }
        finishTrade(dialog, QStringLiteral("已售出 %1 份农产品，收入 %2 金币")
                                .arg(sale.itemsSold).arg(sale.coinsEarned));
    });
    showFarmPanel(dialog);
}

void MainWindow::finishTrade(FarmPanel *dialog, const QString &receipt) {
    updateHud();
    refreshOpenPanel();
    const QString message = saveGame(false) ? receipt : receipt + QStringLiteral("（未保存）");
    dialog->setMessage(message);
    showStatus(message);
}

void MainWindow::showFarmPanel(QDialog *dialog) {
    m_panelOpen = dialog;
    // Panel hotkeys use event filters and can otherwise fire on a disabled widget.
    dialog->installEventFilter(this);
    for (QWidget *child : dialog->findChildren<QWidget *>())
        child->installEventFilter(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &QDialog::finished, this, [this] {
        m_panelOpen.clear();
        refreshGameplayInput();
        // Reactivate the game after Qt has finished dismissing the modal window.
        QTimer::singleShot(0, this, [this] {
            if (isVisible() && !m_panelOpen && !m_transitionActive) {
                activateWindow();
                refreshGameplayInput();
            }
        });
    });
    refreshOpenPanel();
    refreshGameplayInput();
    dialog->open();
}

void MainWindow::refreshOpenPanel() {
    if (auto *bag = qobject_cast<InventoryDialog *>(m_panelOpen.data())) {
        bag->setState(m_economy, itemBar->selectedTool(), currentGameDay());
    } else if (auto *shop = qobject_cast<SeedShopDialog *>(m_panelOpen.data())) {
        shop->setState(m_economy);
    } else if (auto *market = qobject_cast<MarketDialog *>(m_panelOpen.data())) {
        market->setState(m_economy);
    }
}

void MainWindow::refreshGameplayInput() {
    const bool blocked = m_panelOpen || m_transitionActive;
    character->setInputBlocked(blocked);
    mapWidget->setInputBlocked(blocked);
    itemBar->setEnabled(!blocked);
    if (m_panelOpen) {
        m_panelOpen->setEnabled(!m_transitionActive);
    }
    if (!blocked) {
        m_gameView->setFocus(Qt::OtherFocusReason);
        character->setFocus(Qt::OtherFocusReason);
    }
}

bool MainWindow::saveGame(bool showError) {
    if (!m_saveEnabled) {
        return false;
    }
    GameSaveData data;
    data.gameMinutes = m_committedMinutes;
    data.lastProcessedDay = m_lastProcessedDay;
    data.playerPosition = character->absolutePosition();
    data.economy = m_economy.state();
    data.world = mapWidget->worldSnapshot();

    QString errorMessage;
    const auto compatible = [this](const GameSaveData &candidate) {
        return isCompatibleSave(candidate);
    };
    if (!SaveManager::save(SaveManager::defaultSavePath(), data, &errorMessage, compatible)) {
        if (showError) {
            // A world action can emit its success feedback immediately after worldChanged.
            QTimer::singleShot(0, this, [this, errorMessage] {
                showStatus(QStringLiteral("保存失败：%1").arg(errorMessage));
            });
        }
        return false;
    }
    return true;
}

void MainWindow::loadGame() {
    const QString current = SaveManager::defaultSavePath();
    const QString legacy = SaveManager::legacySavePath();
    const auto hasSave = [](const QString &path) {
        return QFileInfo::exists(path) || QFileInfo::exists(path + QStringLiteral(".bak"));
    };
    const bool hasCurrent = hasSave(current);
    const QString source = hasCurrent ? current : legacy;
    if (!hasSave(source)) {
        showStatus(QStringLiteral("WASD 移动，滚轮选工具。Tab 背包，E 互动。"));
        return;
    }

    for (const QString &path : {source, source + QStringLiteral(".bak")}) {
        GameSaveData data;
        if (!SaveManager::load(path, &data) || !restoreGame(data)) {
            continue;
        }
        showStatus(path == source ? QStringLiteral("已读取农场存档")
                                  : QStringLiteral("已从备份恢复农场"));
        return;
    }
    if (hasCurrent) {
        m_saveEnabled = false;
        showStatus(QStringLiteral("农场存档无法恢复，已保留文件并暂停保存"));
    } else {
        showStatus(QStringLiteral("已保留旧存档，新农场将另行保存"));
    }
}

bool MainWindow::isCompatibleSave(const GameSaveData &data) const {
    const int maxX = mapWidget->mapWidth() * mapWidget->tileWidth() - character->width();
    const int maxY = mapWidget->mapHeight() * mapWidget->tileHeight() - character->height();
    const auto &layer = data.world.cultivatedLayer;
    return !layer.isEmpty() && layer.size() == mapWidget->mapHeight() &&
           layer.first().size() == mapWidget->mapWidth() &&
           data.playerPosition.x() >= 0 && data.playerPosition.x() <= maxX &&
           data.playerPosition.y() >= 0 && data.playerPosition.y() <= maxY &&
           data.economy.inventoryCapacity == GameConstants::InitialInventoryCapacity;
}

bool MainWindow::restoreGame(const GameSaveData &data) {
    if (!isCompatibleSave(data)) {
        return false;
    }
    EconomyService candidate;
    if (!candidate.restore(data.economy) || !mapWidget->restoreWorld(data.world)) {
        return false;
    }
    m_economy = std::move(candidate);
    m_gameClock.setElapsedGameMinutes(data.gameMinutes);
    m_lastProcessedDay = data.lastProcessedDay;
    character->setInitialPosition(data.playerPosition);
    return true;
}

// 重写鼠标按下事件（在头文件中声明）
void MainWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        // 点击时切换为点击样式图标
        this->setCursor(clickCursor);
    }
    QMainWindow::mousePressEvent(event);
}

// 重写鼠标释放事件（在头文件中声明）
void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        // 释放时恢复初始样式图标
        this->setCursor(customCursor);
    }
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    applyClock(ClockEvent::Save);
    QMainWindow::closeEvent(event);
}
