//mainwinow.cpp
#include "mainwindow.h"
#include "gameconstants.h"
#include "mapwidget.h"
#include "seedshopdialog.h"
#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QFrame>
#include <QGraphicsProxyWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QShortcut>
#include <QTimer>
#include <QUrl>
#include <QWheelEvent>
#include <QtMath>
#include <limits>


MainWindow::MainWindow(QWidget *parent, const QString &savePath, CalendarRules newGameCalendar)
    : QMainWindow(parent)
    , m_gameClock(GameConstants::RealMillisecondsPerGameMinute)
    , m_calendarRules(newGameCalendar.isValid() ? newGameCalendar : CalendarRules{})
    , m_economy(GameConstants::InitialInventoryCapacity)
    , m_savePath(savePath.isEmpty() ? SaveManager::defaultSavePath() : savePath)
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
    m_sleepSound = new SleepSoundCue(this);


    // No tick or autosave is allowed before the complete state is restored.
    m_gameClock.pause();
    m_gameClock.setElapsedGameMinutes(360); // A new playable day begins at 06:00.
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
    m_gameView->viewport()->installEventFilter(this);
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
    m_bedHintLabel = new QLabel(QStringLiteral("E - REST BEFORE 02:00"), overlay);
    m_bedHintLabel->setFont(digitalFont);
    m_bedHintLabel->setStyleSheet(hudStyle);
    m_bedHintLabel->setAlignment(Qt::AlignCenter);
    m_bedHintLabel->setGeometry(156, 204, 208, 20);
    m_bedHintLabel->hide();
    m_coinsLabel->setGeometry(8, 8, 112, 24);
    m_inventoryLabel->setGeometry(8, 34, 112, 24);
    m_cropsLabel->setGeometry(8, 60, 180, 24);
    m_seedsLabel->setGeometry(8, 86, 180, 24);
    m_statusLabel->setGeometry(90, mapWidget->height() - 92, 340, 24);


    // ==================== 角色控件初始化 ====================
    character = new ImageMovementWidget(mapWidget, mapWidget);
    QPoint initialPos(250, 170);
    character->setInitialPosition(initialPos);
    // 初始位置设置为地图中心（根据地图尺寸动态计算）
    // QPoint initialPos(500, 200);
    // character->setInitialPosition(initialPos);



    // World tint covers the map and character. The toolbar is created above it;
    // canvas-level HUD/panels remain above the entire world subtree.
    m_dayNightOverlay = new DayNightOverlay(mapWidget);
    m_dayNightOverlay->setGeometry(mapWidget->rect());

    // 初始化物品栏（父控件为 mapWidget）
    itemBar = new ItemBar(mapWidget);
    itemBar->raise(); // 显示在最上层

    // 设置物品栏位置和尺寸（221x54）
    const int itemBarWidth = 221;
    const int itemBarHeight = 54;
    itemBar->setGeometry(
        (mapWidget->width() - itemBarWidth) / 2, // 水平居中
        mapWidget->height() - itemBarHeight - 10, // 距离底部 10 像素
        itemBarWidth,
        itemBarHeight
        );


    // 连接物品栏的索引变化信号到角色工具切换
    connect(itemBar, &ItemBar::itemSelected,
            character, &ImageMovementWidget::setCurrentTool);

    connect(mapWidget, &MapWidget::mapClicked,
            character, &ImageMovementWidget::useCurrentToolAt);

    mapWidget->setHarvestHandler([this](CropType type, int quantity) {
        const bool collected = m_economy.collectHarvest(type, quantity);
        if (collected) {
            updateHud();
        }
        return collected;
    });
    mapWidget->setPlantHandler(
        [this](CropType type, const std::function<bool()> &plantAction) {
            const bool planted = m_economy.plantWithSeed(type, plantAction);
            if (planted) {
                updateHud();
            }
            return planted;
        });

    connect(mapWidget, &MapWidget::cropHarvested,
            [this](CropType type, int quantity) {
                showStatus(QStringLiteral("+%1 %2")
                               .arg(quantity)
                               .arg(CropCatalog::definition(type).displayName));
            });
    connect(mapWidget, &MapWidget::harvestBlocked,
            [this](CropType, int requiredCapacity) {
                showStatus(QStringLiteral("BAG FULL - NEED %1 SLOTS")
                               .arg(requiredCapacity));
            });
    connect(mapWidget, &MapWidget::plantingBlocked,
            [this](CropType type) {
                showStatus(QStringLiteral("NO %1 SEEDS - VISIT SEED SHOP")
                               .arg(CropCatalog::definition(type).displayName.toUpper()));
            });

    connect(character, &ImageMovementWidget::interactionRequested,
            [this](const QPoint &playerTile) {
                if (m_sleepBusy || !m_ready) return;
                if (enforcePassOut()) return;
                if (m_shopArea.contains(playerTile)) {
                    openSeedShop();
                    return;
                }
                if (m_bedArea.contains(playerTile)) {
                    requestSleep();
                    return;
                }
                if (m_marketArea.contains(playerTile)) {
                    const SaleResult sale =
                        m_economy.sellAll(playerTile, m_marketArea);
                    if (!sale.success) {
                        showStatus(QStringLiteral("BAG IS EMPTY"));
                        return;
                    }
                    updateHud();
                    saveGame(false);
                    showStatus(QStringLiteral("SOLD %1 ITEMS  +%2 COINS")
                                   .arg(sale.itemsSold)
                                   .arg(sale.coinsEarned));
                    return;
                }
                showStatus(QStringLiteral("STAND BY A MARKER AND PRESS E"));
            });
    connect(mapWidget, &MapWidget::worldChanged,
            this, [this]() { saveGame(false); });




    // 加载背景
    loadBackground();

    // == 创建时间标签 ==




    // ==================== 时间标签初始化 ====================
    timeLabel->setGeometry(
        overlay->width() - 125,
        12,
        110,
        30
        );
    timeLabel->setStyleSheet("color: white;");
    timeLabel->setFont(digitalFont);
    timeLabel->setAlignment(Qt::AlignCenter);
    timeLabel->show();

    QTimer *timeUpdateTimer = new QTimer(this);
    connect(timeUpdateTimer, &QTimer::timeout, this, &MainWindow::updateTime);
    timeUpdateTimer->start(1000); // 每秒更新一次
    updateTime(); // 立即显示初始时间

    // ==================== 天数标签 ====================
    dayLabel->setStyleSheet("color: white;");
    m_calendarFont = CalendarPresentation::chooseFont();
    dayLabel->setFont(m_calendarFont.font);
    dayLabel->setAlignment(Qt::AlignCenter);
    dayLabel->setGeometry(mapWidget->width() - 125, 50, 110, 32);


    // 为窗口安装事件过滤器
    this->installEventFilter(this);

    // 将时间系统传递给mapWidget（需要先在MapWidget类中添加接收方法）
    mapWidget->setTimeReference([this]() {
        return m_gameClock.elapsedGameMinutes();
    });
    const bool hadSave = QFileInfo::exists(m_savePath) ||
                         QFileInfo::exists(m_savePath + ".bak");
    const bool loadedSave = loadGame();

    QTimer *autosaveTimer = new QTimer(this);
    connect(autosaveTimer, &QTimer::timeout,
            this, [this]() { saveGame(false); });
    autosaveTimer->start(60000);

    m_inventoryPanel = new InventoryPanel(m_gameCanvas);
    m_inventoryPanel->setFont(digitalFont);
    connect(m_inventoryPanel, &InventoryPanel::closeRequested,
            this, &MainWindow::toggleInventory);
    QShortcut *backpackShortcut = new QShortcut(QKeySequence(Qt::Key_B), this);
    backpackShortcut->setAutoRepeat(false);
    connect(backpackShortcut, &QShortcut::activated, this, &MainWindow::toggleInventory);
    QShortcut *closeBackpackShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    closeBackpackShortcut->setAutoRepeat(false);
    connect(closeBackpackShortcut, &QShortcut::activated, this, [this]() {
        if (m_sleepConfirming) { cancelSleepConfirmation(); return; }
        if (m_sleepBusy) return;
        if (!m_inventoryPanel->isHidden()) toggleInventory();
    });

    m_sleepOverlay = new SleepOverlay(m_gameCanvas);
    m_sleepOverlay->setFont(digitalFont);
    connect(m_sleepOverlay, &SleepOverlay::confirmed, this, &MainWindow::confirmSleep);
    connect(m_sleepOverlay, &SleepOverlay::cancelled, this, &MainWindow::cancelSleepConfirmation);
    connect(m_sleepOverlay, &SleepOverlay::settlementRequested, this, &MainWindow::completeSleep);
    connect(m_sleepOverlay, &SleepOverlay::retrySaveRequested, this, [this]() {
        if (m_sleepOverlay->phase() != SleepOverlay::Phase::Failed &&
            m_sleepOverlay->phase() != SleepOverlay::Phase::Morning) return;
        if (m_sleepOverlay->phase() == SleepOverlay::Phase::Failed) {
            completeSleep();
            return;
        }
        m_sleepSaved = saveGame(true);
        m_sleepOverlay->setSaveResult(m_sleepSaved);
        m_sleepSound->complete(m_sleepSettled, m_sleepSaved);
    });
    connect(m_sleepOverlay, &SleepOverlay::finished, this, [this]() {
        m_sleepSound->finishAttempt();
        m_sleepBusy = false;
        m_gameClock.resume();
        mapWidget->setEnabled(true);
        character->setFocus();
        updateTime();
        showStatus(!m_sleepSettled ? QStringLiteral("SLEEP CANCELLED - DAY NOT COMPLETED")
                   : m_sleepSaved ? QStringLiteral("GOOD MORNING - PROGRESS SAVED")
                                : QStringLiteral("SAVE FAILED - PROGRESS NOT SAVED"));
    });

    // Embed the completed canvas only after every child widget has been
    // constructed. Children created after an already-visible parent remain
    // hidden by default, which would make the player and HUD disappear.
    QGraphicsProxyWidget *gameProxy = m_gameScene->addWidget(m_gameCanvas);
    gameProxy->setCacheMode(QGraphicsItem::NoCache);
    updateGameScale();
    // An overdue save may trigger pass-out immediately. All UI must exist first.
    m_ready = true;
    m_lastObservedGameMinutes = m_gameClock.elapsedGameMinutes();
    updateTime();
    if (!m_sleepBusy) m_gameClock.resume();
    updateHud();
    if (!m_sleepBusy) {
        if (loadedSave) showStatus(QStringLiteral("SAVE LOADED"));
        else if (!hadSave) showStatus(QStringLiteral("SELL  |  SEEDS  |  SLEEP - PRESS E"));
    }
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
    if (!m_ready) return;
    if (!m_sleepBusy && !m_processingDays) enforcePassOut();
    if (!m_sleepBusy) processDayTransitions();
    // One time source for natural ticks, load restoration and sleep jumps.
    QString gameTime = QString("%1:%2")
    .arg(currentGameHour(), 2, 10, QLatin1Char('0'))
        .arg(currentGameMinute(), 2, 10, QLatin1Char('0'));
    timeLabel->setText(gameTime);

    if (!m_dayAdvanceFailed) refreshCalendar();
    m_dayNightOverlay->setGameTime(m_gameClock.elapsedGameMinutes(), true, m_displayedDate.season);
    m_bedHintLabel->setVisible(!m_sleepBusy && mapWidget->isEnabled() &&
                              m_bedArea.contains(character->currentTilePosition()));
}

bool MainWindow::enforcePassOut() {
    if (!m_ready || m_sleepBusy || m_processingDays || m_enforcingPassOut) return false;
    const qint64 now = m_gameClock.elapsedGameMinutes();
    const auto deadline = GameClock::passOutDeadline(m_lastObservedGameMinutes, now);
    m_lastObservedGameMinutes = now;
    if (!deadline) return false;
    m_enforcingPassOut = true;
    m_gameClock.setElapsedGameMinutes(*deadline); // Stop at the first missed deadline.
    m_lastObservedGameMinutes = *deadline;
    startSleep(true);
    m_enforcingPassOut = false;
    return m_sleepBusy;
}

std::optional<QPoint> MainWindow::bedWakePosition() const {
    // Prefer a tile inside the existing bed marker; then its nearby perimeter.
    for (int radius = 0; radius <= 4; ++radius) {
        const QRect area = m_bedArea.adjusted(-radius, -radius, radius, radius);
        for (int y = area.bottom(); y >= area.top(); --y) {
            for (int x = area.left(); x <= area.right(); ++x) {
                if (radius && m_bedArea.adjusted(-radius + 1, -radius + 1,
                                                radius - 1, radius - 1).contains(x, y)) continue;
                const QPoint position(x * mapWidget->tileWidth() + mapWidget->tileWidth() / 2 - character->width() / 2,
                                      y * mapWidget->tileHeight() + mapWidget->tileHeight() / 2 - character->height() / 2);
                if (character->canStandAt(position)) return position;
            }
        }
    }
    return std::nullopt;
}

void MainWindow::refreshCalendar() {
    const auto date = GameCalendar::forDay(m_gameClock.elapsedGameMinutes() / 1440, m_calendarRules);
    if (!date) return;
    const bool changedSeason = m_hasDisplayedDate &&
        (date->season != m_displayedDate.season || date->year != m_displayedDate.year);
    const bool refreshVisuals = !m_hasDisplayedDate || date->season != m_displayedDate.season;
    m_displayedDate = *date;
    m_hasDisplayedDate = true;
    const QString text = CalendarPresentation::dateText(*date, m_calendarFont.chinese, true);
    QFont font = m_calendarFont.font;
    while (font.pointSizeF() > 6 &&
           (QFontMetrics(font).horizontalAdvance(text.section('\n', 0, 0)) > dayLabel->width() ||
            QFontMetrics(font).horizontalAdvance(text.section('\n', 1, 1)) > dayLabel->width()))
        font.setPointSizeF(font.pointSizeF() - 0.5);
    dayLabel->setFont(font);
    dayLabel->setText(text);
    dayLabel->setToolTip(CalendarPresentation::dateText(*date, m_calendarFont.chinese));
    m_dayNightOverlay->setProperty("seasonId", GameCalendar::seasonKey(date->season));
    if (refreshVisuals) {
        QString error;
        const bool applied = mapWidget->setSeasonVisuals(SeasonVisuals::profile(date->season), &error);
        m_dayNightOverlay->setProperty("seasonVisualFallback", !applied);
        if (!applied) qWarning() << "Season texture fallback; previous world retained:" << error;
    }
    if (changedSeason) emit seasonChanged(date->year, static_cast<int>(date->season));
}

DayAdvanceResult MainWindow::processDayTransitions(bool saveAfter) {
    DayAdvanceResult total;
    if (!m_ready || m_processingDays) return total;
    m_dayAdvanceFailed = false;
    const WorldStateSnapshot before = mapWidget->worldSnapshot();
    if (before.lastCompletedDay != m_lastProcessedDay - 1 ||
        m_lastProcessedDay > currentGameDay()) {
        m_dayAdvanceFailed = true;
        showStatus(QStringLiteral("DAY STATE ERROR - SAVE BLOCKED"));
        return total;
    }
    const bool wasPaused = m_gameClock.isPaused();
    m_gameClock.pause();
    m_processingDays = true;
    const int previousProcessedDay = m_lastProcessedDay;
    const int currentDay = currentGameDay();
    bool processedDay = false;
    while (m_lastProcessedDay < currentDay) {
        const DayAdvanceResult result = mapWidget->advanceDay(m_lastProcessedDay);
        if (mapWidget->worldSnapshot().lastCompletedDay != m_lastProcessedDay) {
            mapWidget->restoreWorld(before);
            m_lastProcessedDay = previousProcessedDay;
            m_dayAdvanceFailed = true;
            total = {};
            showStatus(QStringLiteral("DAY SETTLEMENT FAILED - SAVE BLOCKED"));
            break;
        }
        total.cropsAdvanced += result.cropsAdvanced;
        total.cropsStalled += result.cropsStalled;
        total.soilRecovered += result.soilRecovered;
        ++m_lastProcessedDay;
        processedDay = true;
    }
    m_processingDays = false;
    if (processedDay && !m_dayAdvanceFailed && saveAfter && !m_sleepBusy) {
        saveGame(false);
    }
    if (!wasPaused) m_gameClock.resume();
    return total;
}

void MainWindow::requestSleep() {
    if (!m_ready || m_sleepBusy || !mapWidget->isEnabled() ||
        !m_bedArea.contains(character->currentTilePosition())) return;
    if (enforcePassOut()) return;
    processDayTransitions();
    if (m_dayAdvanceFailed) return;
    m_confirmationWasPaused = m_gameClock.isPaused();
    m_gameClock.pause();
    const auto target = GameClock::sleepTargetMinute(m_gameClock.elapsedGameMinutes(), GameConstants::WakeUpHour);
    if (!target || *target / 1440 > std::numeric_limits<int>::max()) {
        if (!m_confirmationWasPaused) m_gameClock.resume();
        showStatus(QStringLiteral("CANNOT ADVANCE TO MORNING"));
        return;
    }
    m_sleepConfirming = true;
    m_sleepBusy = true;
    character->stopMovement();
    mapWidget->setEnabled(false);
    m_bedHintLabel->hide();
    const auto date = GameCalendar::forDay(*target / 1440, m_calendarRules);
    m_sleepOverlay->showConfirmation(CalendarPresentation::dateText(*date, false) + QStringLiteral(" 06:00"));
}

void MainWindow::cancelSleepConfirmation() {
    if (!m_sleepConfirming) return;
    m_sleepConfirming = false;
    m_sleepBusy = false;
    m_sleepOverlay->dismissConfirmation();
    if (!m_confirmationWasPaused) m_gameClock.resume();
    mapWidget->setEnabled(true);
    character->setFocus();
    updateTime();
    showStatus(QStringLiteral("REST CANCELLED"));
}

void MainWindow::confirmSleep() {
    if (!m_sleepConfirming) return;
    if (!m_ready || m_processingDays || !m_bedArea.contains(character->currentTilePosition())) {
        cancelSleepConfirmation();
        showStatus(QStringLiteral("STAND BY THE BED TO REST"));
        return;
    }
    m_sleepConfirming = false;
    m_sleepBusy = false;
    sleepUntilMorning();
    if (!m_sleepBusy) {
        m_sleepOverlay->dismissConfirmation();
        mapWidget->setEnabled(true);
        if (!m_confirmationWasPaused) m_gameClock.resume();
        character->setFocus();
    }
}

void MainWindow::sleepUntilMorning() {
    startSleep(false);
}

void MainWindow::startSleep(bool passedOut) {
    if (!m_ready || m_sleepBusy) return;
    // Synchronize any midnight reached immediately before the interaction.
    processDayTransitions(false);
    if (m_dayAdvanceFailed && !passedOut) return;
    m_gameClock.pause();
    const auto target = GameClock::sleepTargetMinute(m_gameClock.elapsedGameMinutes(),
                                                    GameConstants::WakeUpHour);
    if (!target || *target / 1440 > std::numeric_limits<int>::max()) {
        m_gameClock.resume();
        showStatus(QStringLiteral("CANNOT ADVANCE TO MORNING"));
        return;
    }
    m_sleepOldDay = currentGameDay();
    m_sleepStartMinutes = m_gameClock.elapsedGameMinutes();
    m_sleepSaved = false;
    m_sleepSettled = false;
    m_sleepTarget = *target;
    m_sleepBusy = true;
    m_passedOut = passedOut;
    if (passedOut) {
        m_inventoryPanel->hide();
        if (m_activeShop) m_activeShop->reject();
    }
    m_sleepSound->beginAttempt();
    m_bedHintLabel->hide();
    character->stopMovement();
    mapWidget->setEnabled(false);
    m_sleepOverlay->begin(passedOut);
}

void MainWindow::completeSleep() {
    if (!m_sleepBusy || m_sleepSettled) return;
    const auto before = mapWidget->worldSnapshot();
    const int beforeProcessedDay = m_lastProcessedDay;
    m_gameClock.setElapsedGameMinutes(m_sleepTarget);
    const DayAdvanceResult result = processDayTransitions(false);
    if (!m_dayAdvanceFailed && m_passedOut) {
        const auto position = bedWakePosition();
        if (position) character->setInitialPosition(*position);
        else {
            mapWidget->restoreWorld(before);
            m_lastProcessedDay = beforeProcessedDay;
            m_dayAdvanceFailed = true;
            showStatus(QStringLiteral("NO SAFE POSITION NEAR BED - RETRY REQUIRED"));
        }
    }
    if (m_dayAdvanceFailed) {
        m_gameClock.setElapsedGameMinutes(m_sleepStartMinutes);
        m_lastObservedGameMinutes = m_sleepStartMinutes;
        updateTime();
        m_sleepOverlay->showFailure();
        return;
    }
    m_sleepSettled = true;
    m_lastObservedGameMinutes = m_gameClock.elapsedGameMinutes();
    updateTime();
    m_sleepSaved = saveGame(false);
    m_sleepOverlay->showMorning(m_sleepOldDay, currentGameDay(), result, m_sleepSaved,
                               m_calendarRules);
    m_sleepSound->complete(m_sleepSettled, m_sleepSaved);
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
            mapWidget->width() - 125,
            12,                        // 上边距
            110,
            30                         // 高度
            );
    }
    if (dayLabel && mapWidget)
    {
        dayLabel->setGeometry(
            mapWidget->width() - 125,
            50,                        // 上边距
            110,
            32
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

void MainWindow::openSeedShop() {
    if (m_sleepBusy) return;
    character->stopMovement();
    SeedShopDialog dialog(this);
    m_activeShop = &dialog;
    const auto refreshDialog = [this, &dialog]() {
        dialog.setState(m_economy.coins(),
                        m_economy.seedCount(CropType::Wheat),
                        m_economy.seedCount(CropType::Potato));
    };
    refreshDialog();

    connect(&dialog, &SeedShopDialog::purchaseRequested,
            this, [this, &dialog, refreshDialog](CropType type, int quantity) {
                if (m_sleepBusy || enforcePassOut()) return;
                const SeedPurchaseResult result =
                    m_economy.buySeeds(type, quantity);
                if (!result.success) {
                    dialog.setMessage(
                        result.failure == SeedPurchaseFailure::InsufficientCoins
                            ? QStringLiteral("NOT ENOUGH COINS")
                            : QStringLiteral("INVALID PURCHASE"));
                    return;
                }
                updateHud();
                refreshDialog();
                saveGame(false);
                dialog.setMessage(QStringLiteral("BOUGHT %1 %2 SEEDS  -%3 COINS")
                                      .arg(result.quantity)
                                      .arg(CropCatalog::definition(type)
                                               .displayName.toUpper())
                                      .arg(result.coinsSpent));
            });
    dialog.exec();
    m_activeShop = nullptr;
    if (!m_sleepBusy) character->setFocus();
}

void MainWindow::toggleInventory() {
    if (m_sleepBusy) return;
    const bool opening = m_inventoryPanel->isHidden();
    character->stopMovement();
    mapWidget->setEnabled(!opening);
    if (opening) {
        m_inventoryPanel->setState(m_economy.state());
        m_inventoryPanel->show();
        m_inventoryPanel->raise();
        m_inventoryPanel->setFocus();
    } else {
        m_inventoryPanel->hide();
        character->setFocus();
    }
}

bool MainWindow::saveGame(bool showError) {
    if (!m_saveEnabled || !m_ready || m_processingDays || m_sleepConfirming) {
        return false;
    }
    if (m_sleepBusy && !m_sleepSettled && m_sleepOverlay->phase() == SleepOverlay::Phase::Failed)
        return false;
    if (!m_sleepBusy && !m_enforcingPassOut && enforcePassOut()) return false;
    const bool wasPaused = m_gameClock.isPaused();
    m_gameClock.pause();
    processDayTransitions(false);
    if (m_dayAdvanceFailed) {
        if (!wasPaused) m_gameClock.resume();
        return false;
    }
    GameSaveData data;
    data.calendarRules = m_calendarRules;
    data.gameMinutes = m_gameClock.elapsedGameMinutes();
    data.lastProcessedDay = m_lastProcessedDay;
    data.playerPosition = character->absolutePosition();
    data.economy = m_economy.state();
    data.world = mapWidget->worldSnapshot();

    QString errorMessage;
    const bool saved = SaveManager::save(m_savePath, data, &errorMessage);
    if (!wasPaused) m_gameClock.resume();
    if (!saved) {
        if (showError) {
            showStatus(QStringLiteral("SAVE FAILED: %1").arg(errorMessage));
        }
        return false;
    }
    return true;
}

bool MainWindow::loadGame() {
    const QString savePath = m_savePath;
    if (!QFileInfo::exists(savePath) && !QFileInfo::exists(savePath + ".bak")) {
        return false;
    }

    GameSaveData data;
    QString errorMessage;
    bool loaded = SaveManager::load(savePath, &data, &errorMessage);
    if (!loaded) {
        loaded = SaveManager::load(savePath + ".bak", &data, &errorMessage);
    }
    const qint64 currentDay = data.gameMinutes / (24 * 60);
    const int mapPixelWidth = mapWidget->mapWidth() * mapWidget->tileWidth();
    const int mapPixelHeight = mapWidget->mapHeight() * mapWidget->tileHeight();
    if (!loaded || currentDay > std::numeric_limits<int>::max() || data.lastProcessedDay > currentDay ||
        data.world.lastCompletedDay != data.lastProcessedDay - 1 ||
        data.playerPosition.x() < 0 || data.playerPosition.y() < 0 ||
        data.playerPosition.x() + character->width() > mapPixelWidth ||
        data.playerPosition.y() + character->height() > mapPixelHeight) {
        m_saveEnabled = false;
        showStatus(QStringLiteral("SAVE LOAD FAILED - ORIGINAL PRESERVED: %1")
                       .arg(errorMessage));
        return false;
    }

    const EconomyState oldEconomy = m_economy.state();
    const WorldStateSnapshot oldWorld = mapWidget->worldSnapshot();
    if (!m_economy.restore(data.economy) ||
        !mapWidget->restoreWorld(data.world)) {
        m_economy.restore(oldEconomy);
        mapWidget->restoreWorld(oldWorld);
        m_saveEnabled = false;
        showStatus(QStringLiteral("SAVE LOAD FAILED: INVALID GAME STATE"));
        return false;
    }

    m_gameClock.setElapsedGameMinutes(data.gameMinutes);
    m_lastObservedGameMinutes = data.gameMinutes;
    m_calendarRules = data.calendarRules;
    m_lastProcessedDay = data.lastProcessedDay;
    character->setInitialPosition(data.playerPosition);
    return true;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == this && m_sleepBusy && (event->type() == QEvent::Wheel ||
                        event->type() == QEvent::MouseButtonPress)) return true;
    if (watched == m_gameView->viewport()) {
        if (event->type() == QEvent::Resize) {
            QTimer::singleShot(0, this, &MainWindow::updateGameScale);
        }
        return QMainWindow::eventFilter(watched, event);
    }
    if (event->type() == QEvent::Wheel) {
        itemBar->wheelEvent(static_cast<QWheelEvent*>(event));
        return true; // 拦截滚轮事件
    }

    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            qDebug() << "[MainWindow] 鼠标点击位置（窗口坐标）:" << mouseEvent->pos();

            // 获取地图控件的滚动偏移
            int mapOffsetX = mapWidget->offsetX();
            int mapOffsetY = mapWidget->offsetY();

            // 计算实际点击位置在地图中的全局坐标
            QPoint mapGlobalPos = mapWidget->mapToGlobal(QPoint(0, 0));
            QPoint actualMapPos = mouseEvent->pos() + QPoint(mapOffsetX, mapOffsetY);

            // 转换为角色控件的局部坐标
            QPoint characterLocalPos = character->mapFromGlobal(actualMapPos + mapGlobalPos);
            if (character->rect().contains(characterLocalPos)) {
                qDebug() << "点击在角色控件内";
                character->handleToolAction();
            }
            return false; // 允许事件继续传递
        }
    }
    return QMainWindow::eventFilter(watched, event);
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
    if (m_sleepConfirming) cancelSleepConfirmation();
    m_sleepSound->finishAttempt();
    saveGame(false);
    QMainWindow::closeEvent(event);
}

// ==================== 程序入口 ====================
#ifndef CLOVER_COG_TESTING
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("CloverCog"));
    QApplication::setApplicationName(QStringLiteral("Clover-Cog"));


    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
#endif
