//mainwinow.cpp
#include "mainwindow.h"
#include "gameconstants.h"
#include "inventorydialog.h"
#include "mapwidget.h"
#include "marketdialog.h"
#include "seedshopdialog.h"
#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFrame>
#include <QGraphicsProxyWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTimer>
#include <QUrl>
#include <QtMath>
#include <utility>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_gameClock(GameConstants::RealMillisecondsPerGameMinute)
    , m_economy(GameConstants::InitialInventoryCapacity)
    , m_gameCanvas(new QWidget)
    , m_gameScene(new QGraphicsScene(this))
    , m_gameView(new QGraphicsView(m_gameScene, this))
    , mapWidget(new MapWidget(m_gameCanvas))
    , overlay(new QWidget(m_gameCanvas))
    , clockBgWidget(new ClockBgWidget(m_gameCanvas))
    ,timeLabel(new QLabel(overlay))
    ,dayLabel(new QLabel(overlay))
    , customCursor(QPixmap(":/ui/sprout/Catpaw Mouse icon.png"), 8, 8)  // 初始化成员变量
    , clickCursor(QPixmap(":/ui/sprout/Catpaw holding Mouse icon.png"), 8, 8)
{

    // ==================== 鼠标设置 ====================
    // 设置自定义鼠标样式
    this->setCursor(customCursor);


    // ==================== 音乐设置 ====================

    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_audioOutput->setVolume(0.5);
    m_player->setAudioOutput(m_audioOutput);

    // 监听媒体状态变化
    connect(m_player, &QMediaPlayer::mediaStatusChanged, [this](QMediaPlayer::MediaStatus status) {
        qDebug() << "MediaStatus:" << status;
        if (status == QMediaPlayer::LoadedMedia) {
            qDebug() << "开始播放...";
            m_player->play();
        }
        else if (status == QMediaPlayer::EndOfMedia) {
            // 当音乐播放结束时，重新播放
            m_player->play();
        }
    });

    // 监听播放状态变化
    connect(m_player, &QMediaPlayer::playbackStateChanged, [](QMediaPlayer::PlaybackState state) {
        qDebug() << "PlaybackState:" << state;
    });

    // 设置资源
    QString filePath = "qrc:/sounds/wav/On_the_Farm.wav";
    m_player->setSource(QUrl(filePath));


    m_gameClock.setElapsedGameMinutes(GameConstants::WakeUpHour * 60);
    m_gameClock.start();
    // ==================== 窗口基础设置 ====================

    constexpr int logicalWidth = 520;
    constexpr int logicalHeight = 320;
    resize(logicalWidth, logicalHeight);

    m_gameCanvas->setFixedSize(logicalWidth, logicalHeight);
    QPalette canvasPalette = m_gameCanvas->palette();
    canvasPalette.setColor(QPalette::Window, Qt::black);
    m_gameCanvas->setPalette(canvasPalette);
    m_gameCanvas->setAutoFillBackground(true);
    m_gameScene->setSceneRect(0, 0, logicalWidth, logicalHeight);

    m_gameView->setFrameShape(QFrame::NoFrame);
    m_gameView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gameView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gameView->setAlignment(Qt::AlignCenter);
    m_gameView->setBackgroundBrush(QColor(24, 24, 24));
    m_gameView->setRenderHint(QPainter::SmoothPixmapTransform, false);
    m_gameView->setTransformationAnchor(QGraphicsView::NoAnchor);
    m_gameView->setResizeAnchor(QGraphicsView::NoAnchor);
    setCentralWidget(m_gameView);

    // 地图控件尺寸与主窗口一致
    mapWidget->setGeometry(0, 0, logicalWidth, logicalHeight);
    if (!mapWidget->loadMap(":/map/map6.tmj")) {
        qCritical() << "Unable to initialize the game map";
    }
    mapWidget->lower();
    mapWidget->setMarketArea(m_marketArea);
    mapWidget->setShopArea(m_shopArea);
    mapWidget->setBedArea(m_bedArea);

    // ==================== 时钟背景层设置 ====================
    clockBgWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    clockBgWidget->setAttribute(Qt::WA_NoSystemBackground);
    clockBgWidget->setGeometry(0, 0, logicalWidth, logicalHeight);
    clockBgWidget->raise(); // 关键修改：将覆盖层置底

    // ==================== 覆盖层设置 ====================
    overlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    overlay->setAttribute(Qt::WA_NoSystemBackground);
    overlay->setGeometry(0, 0, logicalWidth, logicalHeight);
    overlay->raise(); // 关键修改：将覆盖层置底

    loadCustomFont();
    const QString hudStyle = QStringLiteral(
        "color: white; background-color: rgba(76, 48, 30, 190);"
        "border: 1px solid #d9ae68; padding: 2px;");
    m_coinsLabel = new QLabel(overlay);
    m_inventoryLabel = new QLabel(overlay);
    m_cropsLabel = new QLabel(overlay);
    m_seedsLabel = new QLabel(overlay);
    m_statusLabel = new QLabel(overlay);
    for (QLabel *label : {m_coinsLabel, m_inventoryLabel, m_cropsLabel,
                          m_seedsLabel, m_statusLabel}) {
        label->setStyleSheet(hudStyle);
        label->setFont(digitalFont);
    }
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_coinsLabel->setGeometry(8, 8, 112, 24);
    m_inventoryLabel->setGeometry(8, 34, 112, 24);
    m_cropsLabel->setGeometry(8, 60, 180, 24);
    m_seedsLabel->setGeometry(8, 86, 180, 24);
    m_statusLabel->setGeometry(90, mapWidget->height() - 92, 340, 24);


    // ==================== 角色控件初始化 ====================
    character = new ImageMovementWidget(mapWidget, mapWidget);
    QPoint initialPos(400, 192);
    character->setInitialPosition(initialPos);
    // 初始位置设置为地图中心（根据地图尺寸动态计算）
    // QPoint initialPos(500, 200);
    // character->setInitialPosition(initialPos);



    // 初始化物品栏（父控件为 mapWidget）
    itemBar = new ItemBar(mapWidget);
    itemBar->raise(); // 显示在最上层

    const QSize barSize = itemBar->sizeHint();
    itemBar->setGeometry((mapWidget->width() - barSize.width()) / 2,
                         mapWidget->height() - barSize.height() - 10,
                         barSize.width(), barSize.height());

    // 连接物品栏的索引变化信号到角色工具切换
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

    // 加载背景
    loadBackground();

    // == 创建时间标签 ==




    // ==================== 时间标签初始化 ====================
    timeLabel->setGeometry(
        overlay->width() - 20, // 右边距
        10,
        110,
        30
        );
    timeLabel->setStyleSheet("color: white;");
    timeLabel->setFont(digitalFont);
    timeLabel->show();

    QTimer *timeUpdateTimer = new QTimer(this);
    connect(timeUpdateTimer, &QTimer::timeout, this, &MainWindow::updateTime);
    timeUpdateTimer->start(1000); // 每秒更新一次
    updateTime(); // 立即显示初始时间

    // ==================== 天数标签 ====================
    dayLabel->setGeometry(
        timeLabel->x() - 100 - 10,     // 时间标签左侧10px
        10,                           // 与时间标签对齐
        100,                          // 宽度
        30                            // 高度
        );
    dayLabel->setStyleSheet("color: white;");
    dayLabel->setFont(digitalFont);
    dayLabel->setAlignment(Qt::AlignCenter);


    // 将时间系统传递给mapWidget（需要先在MapWidget类中添加接收方法）
    mapWidget->setTimeReference([this]() {
        return m_gameClock.elapsedGameMinutes();
    });
    loadGame();
    updateHud();

    QTimer *autosaveTimer = new QTimer(this);
    connect(autosaveTimer, &QTimer::timeout,
            this, [this]() { saveGame(false); });
    autosaveTimer->start(60000);

    // Embed the completed canvas only after every child widget has been
    // constructed. Children created after an already-visible parent remain
    // hidden by default, which would make the player and HUD disappear.
    QGraphicsProxyWidget *gameProxy = m_gameScene->addWidget(m_gameCanvas);
    gameProxy->setCacheMode(QGraphicsItem::NoCache);
    updateGameScale();
}



bool MainWindow::loadCustomFont() {
    int fontId = QFontDatabase::addApplicationFont(":/tilesets/sprout/pixelFont-7-8x14-sproutLands.ttf");
    if (fontId == -1) return false;
    QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    if (families.isEmpty()) return false;
    digitalFont = QFont(families.at(0), 10);
    return true;
}

void ClockBgWidget::paintEvent(QPaintEvent *event)  {
    Q_UNUSED(event);
    QPainter painter(this);
    if (!clockBg.isNull()) {
        const int margin = 10;
        QSize targetSize(120, 40);
        int xPos = width() - targetSize.width() - margin;
        int yPos = margin;
        painter.drawPixmap(xPos, yPos, clockBg.scaled(targetSize, Qt::KeepAspectRatio));
        painter.drawPixmap(xPos, yPos+38, clockBg.scaled(targetSize, Qt::KeepAspectRatio));
    }
}



void MainWindow::loadBackground() {
    // 加载整张精灵图
    QPixmap bg(":/tilesets/sprout/clockbackground.png");
    if (bg.isNull()) {
        qDebug() << "[ERROR] Clock background not found!";
    } else {
        clockBgWidget->setBackground(bg);  // 关键：将背景图设置到控件
    }
}

void MainWindow::updateTime() {
    QString gameTime = QString("%1:%2")
    .arg(currentGameHour(), 2, 10, QLatin1Char('0'))
        .arg(currentGameMinute(), 2, 10, QLatin1Char('0'));
    timeLabel->setText(gameTime);

    int days = currentGameDay();
    dayLabel->setText(QString("DAY %1").arg(days));
}

DayAdvanceResult MainWindow::processDayTransitions() {
    DayAdvanceResult total;
    const int currentDay = currentGameDay();
    bool processedDay = false;
    while (m_lastProcessedDay < currentDay) {
        const DayAdvanceResult result = mapWidget->advanceDay(m_lastProcessedDay);
        total.cropsAdvanced += result.cropsAdvanced;
        total.cropsStalled += result.cropsStalled;
        total.soilRecovered += result.soilRecovered;
        ++m_lastProcessedDay;
        processedDay = true;
    }
    if (processedDay) {
        refreshOpenPanel();
    }
    return total;
}

void MainWindow::sleepUntilMorning() {
    if (!m_gameClock.canSleep(GameConstants::SleepStartHour)) {
        showStatus(QStringLiteral("CANNOT SLEEP BEFORE 20:00"));
        return;
    }
    if (!m_gameClock.sleepToNextMorning(GameConstants::WakeUpHour,
                                        GameConstants::SleepStartHour)) {
        showStatus(QStringLiteral("CANNOT SLEEP BEFORE 20:00"));
        return;
    }
    const DayAdvanceResult result = processDayTransitions();
    updateTime();
    showStatus(QStringLiteral("GOOD MORNING  GROW +%1  PAUSED %2  SOIL -%3")
                   .arg(result.cropsAdvanced)
                   .arg(result.cropsStalled)
                   .arg(result.soilRecovered));
    saveGame(false);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    updateGameScale();
    // 调整背景图大小
    background = background.scaled(this->size(), Qt::KeepAspectRatioByExpanding);
    // 调整时间标签位置
    if (timeLabel && mapWidget) {
        // 右边距 10px，上边距 10px，标签尺寸 100x30
        timeLabel->setGeometry(
            mapWidget->width() - 102,  // 右边距 10px（110 = 100宽度 + 10右边距）
            12,                        // 上边距
            100,                       // 宽度
            30                         // 高度
            );
    }
    if (dayLabel && mapWidget)
    {
        dayLabel->setGeometry(
            mapWidget->width() - 119,  // 右边距 10px（110 = 100宽度 + 10右边距）
            50,                        // 上边距
            100,                       // 宽度
            30                         // 高度
            );
    }

    if (m_coinsLabel) m_coinsLabel->setGeometry(8, 8, 112, 24);
    if (m_inventoryLabel) m_inventoryLabel->setGeometry(8, 34, 112, 24);
    if (m_cropsLabel) m_cropsLabel->setGeometry(8, 60, 180, 24);
    if (m_seedsLabel) m_seedsLabel->setGeometry(8, 86, 180, 24);
    if (m_statusLabel) m_statusLabel->setGeometry(90, mapWidget->height() - 92, 340, 24);
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
    const qreal pixelScale = availableScale >= 1.0
        ? qMax<qreal>(1.0, qFloor(availableScale))
        : availableScale;

    m_gameView->resetTransform();
    m_gameView->scale(pixelScale, pixelScale);
    m_gameView->centerOn(m_gameScene->sceneRect().center());
}

void MainWindow::updateHud() {
    const Inventory &inventory = m_economy.inventory();
    m_coinsLabel->setText(QStringLiteral("COINS %1").arg(m_economy.coins()));
    m_inventoryLabel->setText(QStringLiteral("BAG %1/%2")
                                  .arg(inventory.usedCapacity())
                                  .arg(inventory.capacity()));
    m_cropsLabel->setText(QStringLiteral("WHEAT %1   POTATO %2")
                              .arg(inventory.count(CropType::Wheat))
                              .arg(inventory.count(CropType::Potato)));
    m_seedsLabel->setText(QStringLiteral("SEEDS  W %1   P %2")
                              .arg(m_economy.seedCount(CropType::Wheat))
                              .arg(m_economy.seedCount(CropType::Potato)));
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
    data.gameMinutes = m_gameClock.elapsedGameMinutes();
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
        showStatus(QStringLiteral("Tab 打开背包，E 与站点互动"));
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
    saveGame(false);
    QMainWindow::closeEvent(event);
}
