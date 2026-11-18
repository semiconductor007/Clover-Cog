#include "mainwindow.h"
#include "ui/farmhud.h"
#include "ui/farmmenus.h"
#include "core/gameconstants.h"
#include "ui/inventorydialog.h"
#include "ui/marketdialog.h"
#include "ui/scenetransition.h"
#include "ui/seedshopdialog.h"
#include "ui/storagedialog.h"

#include <QAudioOutput>
#include <QAudioDevice>
#include <QCloseEvent>
#include <QFrame>
#include <QGraphicsProxyWidget>
#include <QMediaPlayer>
#include <QMediaDevices>
#include <QMessageBox>
#include <QPushButton>
#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScreen>
#include <QSoundEffect>
#include <QShortcut>
#include <QTimer>
#include <QUrl>
#include <QWheelEvent>
#include <QToolTip>
#include <QtMath>
#include <utility>
#include <limits>

namespace {
constexpr int CanvasWidth = 576;
constexpr int CanvasHeight = 442;
constexpr int OverlayMargin = 8;

// Footprints of the furniture on map6, in tile coordinates.
constexpr QRect MarketCounter{28, 16, 2, 2};
constexpr QRect ShopCounter{33, 16, 2, 2};
constexpr QRect StorageChest{31, 20, 1, 1};
constexpr QRect Bed{24, 7, 1, 2};

constexpr QRect stationReach(const QRect &footprint) {
    return footprint.adjusted(-1, -1, 1, 1);
}

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
    setWindowFlag(Qt::FramelessWindowHint);
    auto *fullscreen = new QShortcut(QKeySequence(Qt::Key_F11), this);
    fullscreen->setAutoRepeat(false);
    connect(fullscreen, &QShortcut::activated, this, [this] {
        if (isFullScreen()) showNormal(); else showFullScreen();
        QTimer::singleShot(0, this, &MainWindow::updateGameScale);
    });
    setCursor(customCursor);
    qApp->setStyleSheet(qApp->styleSheet() + QStringLiteral(
        "QToolTip { background-color: #f3dfb8; color: #563b2b; border: 2px solid #a37754; padding: 9px; font-size: 13px; }"));


    m_gameCanvas->setFixedSize(CanvasWidth, CanvasHeight);
    QPalette canvasPalette = m_gameCanvas->palette();
    canvasPalette.setColor(QPalette::Window, QColor("#26372e"));
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
    m_pages = new QStackedWidget(this);
    m_pages->addWidget(m_gameView);
    m_wakeSummary = new WakeSummaryPage(m_pages);
    m_pages->addWidget(m_wakeSummary);
    connect(m_wakeSummary, &WakeSummaryPage::continueRequested, this, &MainWindow::leaveMorningPage);
    setCentralWidget(m_pages);
    m_gameView->installEventFilter(this);
    m_gameView->viewport()->installEventFilter(this);
    m_sleepConfirmation = new SleepConfirmationOverlay(m_gameView->viewport());
    connect(m_sleepConfirmation, &SleepConfirmationOverlay::restRequested, this,
            [this] { finishSleepConfirmation(true); });
    connect(m_sleepConfirmation, &SleepConfirmationOverlay::cancelRequested, this,
            [this] { finishSleepConfirmation(false); });

    mapWidget->setGeometry(m_gameCanvas->rect());
    if (!mapWidget->loadMap(QStringLiteral(":/map/map6.tmj"))) {
        qCritical() << "Unable to initialize the game map";
    }
    mapWidget->setMarketArea(MarketCounter);
    mapWidget->setStorageArea(StorageChest);
    mapWidget->setShopArea(ShopCounter);
    mapWidget->setBedArea(Bed);
    m_seasons.load(mapWidget->originalTilesets());
    mapWidget->setTimeReference([this] { return m_committedMinutes; });

    m_hud = new FarmHud(m_gameCanvas);

    character = new ImageMovementWidget(mapWidget, mapWidget);
    character->setInitialPosition(QPoint(400, 192));
    m_daylight = new DayNightOverlay(mapWidget);
    itemBar = new ItemBar(m_gameCanvas);
    connect(itemBar,&ItemBar::slotSelected,this,[this](int slot){m_economy.setEquippedSlot(slot);});
    connect(itemBar,&ItemBar::stackMoveRequested,this,[this](StackMove move){moveInventoryStack(m_panelOpen,move);});
    const QSize barSize = itemBar->sizeHint();
    itemBar->setGeometry((CanvasWidth - barSize.width()) / 2, CanvasHeight - barSize.height() - 8,
                         barSize.width(), barSize.height());
    mapWidget->installEventFilter(this);
    m_suppliesLabel = new QLabel(m_gameCanvas);
    m_suppliesLabel->hide();
    m_statusLabel = new QLabel(m_gameCanvas);
    m_statusLabel->setObjectName(QStringLiteral("farmStatus"));
    m_statusLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_statusLabel->setTextFormat(Qt::PlainText);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet(QStringLiteral(
        "QLabel { background-color: #304738; color: #fff4d8; border: 1px solid #b3bf94;"
        " border-radius: 4px; padding: 4px 9px; font-size: 13px; }"));
    m_statusLabel->hide();
    m_statusTimer = new QTimer(this);
    m_statusTimer->setSingleShot(true);
    m_statusTimer->setInterval(3500);
    connect(m_statusTimer, &QTimer::timeout, m_statusLabel, &QLabel::hide);

    m_transition = new SceneTransition(m_gameCanvas);
    connect(m_transition, &SceneTransition::blackReached, this, &MainWindow::presentTime);
    connect(m_transition, &SceneTransition::finished, this, [this] {
        m_transitionActive = false;
        presentTime();
        refreshGameplayInput();
        if (m_panelOpen && m_panelFocusBeforeFade)
            m_panelFocusBeforeFade->setFocus(Qt::OtherFocusReason);
        m_panelFocusBeforeFade.clear();
        finishWakeSave();
    });

    m_music = new QMediaPlayer(this);
    auto *output = new QAudioOutput(m_music);
    output->setVolume(0.28f);
    m_music->setAudioOutput(output);
    m_music->setLoops(QMediaPlayer::Infinite);
    m_music->setSource(QUrl(QStringLiteral("qrc:/sounds/wav/On_the_Farm.wav")));

    const auto sound = [this](const QString &name, const QString &url) {
        auto *effect = new QSoundEffect(this);
        effect->setObjectName(name);
        effect->setVolume(0.55f);
        effect->setSource(QUrl(url));
        return effect;
    };
    auto *water = sound(QStringLiteral("wateringSound"), QStringLiteral("qrc:/sounds/water.wav"));
    auto *axeHit=sound(QStringLiteral("axeHitSound"),QStringLiteral("qrc:/sounds/axe_hit.wav"));
    auto *treeFall=sound(QStringLiteral("treeFallSound"),QStringLiteral("qrc:/sounds/tree_fall.wav"));
    connect(mapWidget,&MapWidget::treeChopped,this,[axeHit,treeFall](bool felled){
        auto *effect=felled?treeFall:axeHit;
        if(effect->status()==QSoundEffect::Ready && !effect->isMuted() && effect->volume()>0 &&
           !QMediaDevices::defaultAudioOutput().isNull()) effect->play();
    });
    auto *harvest = sound(QStringLiteral("harvestSound"), QStringLiteral("qrc:/sounds/harvest.wav"));
    auto *wake = sound(QStringLiteral("wakeSound"), QStringLiteral("qrc:/sounds/sleep_success.wav"));
    auto *chestOpen = sound(QStringLiteral("chestOpenSound"), QStringLiteral("qrc:/sounds/chest_open.wav"));
    auto *chestClose = sound(QStringLiteral("chestCloseSound"), QStringLiteral("qrc:/sounds/chest_close.wav"));
    chestOpen->setVolume(0.40f);
    chestClose->setVolume(0.40f);
    connect(mapWidget, &MapWidget::storageChestCue, this, [chestOpen, chestClose](bool opening) {
        auto *effect = opening ? chestOpen : chestClose;
        // Audio is best-effort: no delayed playback after closing or device failure.
        if (effect->status() == QSoundEffect::Ready && !effect->isMuted() &&
            effect->volume() > 0 && !QMediaDevices::defaultAudioOutput().isNull()) {
            effect->play();
        }
    });
    connect(mapWidget, &MapWidget::storageChestOpened, this, &MainWindow::finishStorageOpening);
    wake->setVolume(0.35f);
    wake->setLoopCount(1);
    connect(this, &MainWindow::wakeSaved, wake, [wake] {
        // The sleep/save owner emits once after dawn and a successful save.
        // Drop unavailable or silenced cues; never queue a late success sound.
        if (wake->status() == QSoundEffect::Ready && !wake->isMuted()
                && wake->volume() > 0 && !wake->isPlaying()
                && !QMediaDevices::defaultAudioOutput().isNull()) {
            wake->play();
        }
    });
    connect(mapWidget, &MapWidget::tileWatered, water, &QSoundEffect::play);
    connect(mapWidget, &MapWidget::cropHarvested, harvest, &QSoundEffect::play);
    connect(itemBar, &ItemBar::itemSelected, this, &MainWindow::updateHud);
    connect(itemBar, &ItemBar::itemSelected,
            character, &ImageMovementWidget::setCurrentTool);

    connect(mapWidget, &MapWidget::mapClicked,
            character, &ImageMovementWidget::useCurrentToolAt);

    connect(character, &ImageMovementWidget::inventoryRequested, this, &MainWindow::openInventory);
    mapWidget->setWoodHandler([this](int quantity,const std::function<bool()> &action){
        const bool accepted=m_economy.collectWoodWithAction(quantity,action);
        if(accepted) updateHud();
        return accepted;
    });
    mapWidget->setHarvestHandler([this](CropType type, int quantity) {
        return m_economy.collectHarvest(type, quantity);
    });
    mapWidget->setPlantHandler([this](CropType type, const std::function<bool()> &action) {
        return m_economy.plantWithSeed(type, action);
    });
    mapWidget->setFertilizeHandler([this](const std::function<bool()> &action) {
        return m_economy.fertilizeWithItem(action);
    });
    mapWidget->setWaterHandler([this](const std::function<bool()> &action) {
        if (m_economy.waterRemaining() == 0)
            showStatus(QStringLiteral("水壶空了，走到水井旁按 F 补满"));
        return m_economy.waterWithAction(action);
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
        if (m_page != Page::Farm || m_panelOpen || m_storageOpening || m_transitionActive || m_closePrompt ||
            m_wakeSaveState == WakeSaveState::Failed) {
            return;
        }
        if (stationReach(ShopCounter).contains(playerTile)) {
            openSeedShop();
        } else if (stationReach(MarketCounter).contains(playerTile)) {
            openMarket();
        } else if (stationReach(StorageChest).contains(playerTile)) {
            openStorage();
        } else if (stationReach(Bed).contains(playerTile)) {
            requestSleep();
        } else if (mapWidget->isNearWaterWell(playerTile)) {
            if (m_economy.refillWater()) {
                updateHud();
                if (saveGame())
                    showStatus(QStringLiteral("水壶已补满，水量 %1/%1").arg(m_economy.waterCapacity()));
            } else {
                showStatus(QStringLiteral("水壶已经满了"));
            }
        }
    });
    // MapWidget emits this only after both the world action and the inventory action commit.
    connect(mapWidget, &MapWidget::worldChanged, this, [this] {
        updateHud();
        saveGame();
    });

    m_tick = new QTimer(this);
    m_tick->setInterval(100);
    connect(m_tick, &QTimer::timeout, this, &MainWindow::updateTime);
    m_autosave = new QTimer(this);
    m_autosave->setInterval(60000);
    connect(m_autosave, &QTimer::timeout, this, [this] { applyClock(ClockEvent::Save); });

    character->installEventFilter(this);
    m_gameScene->addWidget(m_gameCanvas)->setCacheMode(QGraphicsItem::NoCache);
    m_menus = new FarmMenus(mapWidget->grab(), m_pages);
    m_pages->addWidget(m_menus);
    connect(m_menus, &FarmMenus::enterRequested, this, &MainWindow::enterFarm);
    connect(m_menus, &FarmMenus::resumeRequested, this, &MainWindow::resumeFarm);
    connect(m_menus, &FarmMenus::saveAndLeaveRequested, this, [this] {
        if (m_page != Page::Pause) return;
        if (saveGame(false)) showHome();
        else m_menus->showPause(m_saveEnabled
            ? QStringLiteral("未能保存，农场仍暂停。请检查存档目录的空间和写入权限。")
            : QStringLiteral("未能保存：原档及备份无法恢复，已保留文件。可继续体验或关闭窗口。"));
    });
    connect(m_menus, &FarmMenus::exitRequested, this, &QWidget::close);
    showHome();
    const QSize available = screen()->availableGeometry().size();
    const bool fitsDouble = available.width() >= CanvasWidth * 2 &&
                            available.height() >= CanvasHeight * 2 + 36;
    resize(m_gameCanvas->size() * (fitsDouble ? 2 : 1));
    updateGameScale();
}

void MainWindow::showHome() {
    m_storageOpening = false;
    mapWidget->resetStorageChest();
    m_page = Page::Home;
    refreshGameplayInput();
    m_music->stop();
    m_pages->setCurrentWidget(m_menus);
    const QString source = SaveManager::preferredSavePath();
    GameSaveData preview;
    QString used;
    const bool canContinue = readFarmSave(source, &preview, &used);
    QString notice;
    if (canContinue && used != source)
        notice = QStringLiteral("可从备份恢复农场。");
    else if (!canContinue && !source.isEmpty())
        notice = source == SaveManager::defaultSavePath()
            ? QStringLiteral("存档和备份无法恢复。可进入农场体验，本次无法保存。")
            : QStringLiteral("旧存档已保留，开始种田将使用新档。");
    m_menus->showHome(canContinue, notice);
}

void MainWindow::enterFarm() {
    if (m_page != Page::Home) return;
    m_page = Page::Farm;
    m_saveEnabled = true;
    m_gameClock.setElapsedGameMinutes(m_committedMinutes);
    loadGame();
    m_economy.ensureAxe();
    updateHud();
    m_pages->setCurrentWidget(m_gameView);
    updateGameScale();
    applyClock(ClockEvent::Load);
    refreshGameplayInput();
    m_tick->start();
    m_autosave->start();
    m_music->play();
}

void MainWindow::pauseFarm() {
    if (m_page != Page::Farm || m_panelOpen || m_storageOpening || m_transitionActive || m_closePrompt ||
        m_morning || m_sleepConfirmation->isVisible() || m_wakeSaveState == WakeSaveState::Failed) return;
    m_gameClock.pause();
    applyClock(ClockEvent::Tick);
    // A key at the first instant of a new season must let its single transition finish.
    if (m_transitionActive || m_bedRecoveryPrompt) {
        if (!m_morning && !m_bedRecoveryPrompt) m_gameClock.resume();
        return;
    }
    m_page = Page::Pause;
    m_tick->stop();
    m_autosave->stop();
    m_music->pause();
    refreshGameplayInput();
    m_pages->setCurrentWidget(m_menus);
    m_menus->showPause();
}

void MainWindow::resumeFarm() {
    if (m_page != Page::Pause) return;
    m_page = Page::Farm;
    m_gameClock.resume();
    m_tick->start();
    m_autosave->start();
    m_pages->setCurrentWidget(m_gameView);
    refreshGameplayInput();
    m_music->play();
}

void MainWindow::updateTime() {
    applyClock(ClockEvent::Tick);
}

bool MainWindow::applyClock(ClockEvent event) {
    if (m_bedRecoveryPrompt) return event != ClockEvent::Save;
    if (m_wakeSaveState == WakeSaveState::Failed)
        return event != ClockEvent::Save;
    if (m_page != Page::Farm || m_closePrompt) return true;
    const qint64 minutes = m_gameClock.elapsedGameMinutes();
    if (minutes < 0 || minutes / 1440 > std::numeric_limits<int>::max()) return false;
    const bool passedOut = event == ClockEvent::PassOut;
    const bool woke = event == ClockEvent::WakeUp || passedOut;
    if (!woke && !m_morning) {
        const auto deadline = GameClock::passOutDeadline(
            event == ClockEvent::Load ? minutes : m_committedMinutes, minutes);
        if (deadline) {
            if (m_transitionActive) return event != ClockEvent::Save;
            return recoverToMorning(*deadline, true);
        }
    }
    if (event == ClockEvent::Tick && minutes == m_committedMinutes) return true;
    const int day = int(minutes / (24 * 60));
    const bool crossedDay = m_lastProcessedDay < day;
    const bool crossedSeason = day / SeasonCalendar::DaysPerSeason !=
                               currentGameDay() / SeasonCalendar::DaysPerSeason;
    if (event != ClockEvent::Load && !m_transitionActive && (woke || crossedSeason)) {
        // Freeze the outgoing farm before changing any crop, HUD or seasonal texture.
        m_transitionActive = m_transition->start();
        if (m_panelOpen) m_panelFocusBeforeFade = m_panelOpen->focusWidget();
        refreshGameplayInput();
    }

    const int previousDay = crossedDay ? m_lastProcessedDay : day;
    m_committedMinutes = minutes;
    const DayAdvanceResult result = processDayTransitions();
    if (woke) {
        m_morning = MorningSummary{result, passedOut, previousDay};
    } else if (crossedDay && event != ClockEvent::Load) {
        showStatus(QStringLiteral("新的一天，作物成长 %1 株，缺水 %2 株。")
                       .arg(result.cropsAdvanced).arg(result.cropsStalled));
    }
    if (!m_transitionActive) presentTime();
    if (woke) {
        const bool saved = saveGame(false);
        // A protected, read-only farm must keep its existing no-write behaviour.
        if (m_saveEnabled) {
            m_wakeSaveState = saved ? WakeSaveState::Saved : WakeSaveState::Failed;
            if (!saved) {
                // Keep the already settled morning even if writing took time.
                m_gameClock.setElapsedGameMinutes(m_committedMinutes);
                m_gameClock.pause();
                m_tick->stop();
                m_autosave->stop();
                refreshGameplayInput();
            }
        }
        finishWakeSave();
        return saved;
    }
    if (crossedDay || event == ClockEvent::Save)
        return saveGame(event != ClockEvent::Save);
    return true;
}

DayAdvanceResult MainWindow::processDayTransitions() {
    const int currentDay = currentGameDay();
    if (m_lastProcessedDay >= currentDay)
        return {};
    const auto result = mapWidget->advanceThroughDay(currentDay - 1);
    m_lastProcessedDay = currentDay;
    return result;
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
        m_hud->setDate(QStringLiteral("第 %1 年 · %2 %3 日")
                              .arg(date.year).arg(SeasonCalendar::seasonName(season))
                              .arg(date.dayOfSeason));
        m_visibleDay = day;
        refreshOpenPanel();
    }
    m_hud->setTime(currentGameHour(), currentGameMinute());
    m_daylight->setGameMinutes(m_committedMinutes);
}

void MainWindow::sleepUntilMorning() {
    if (m_page != Page::Farm || m_transitionActive || m_panelOpen || m_storageOpening || m_closePrompt ||
        m_sleepConfirmation->isVisible() || m_wakeSaveState == WakeSaveState::Failed ||
        !stationReach(Bed).contains(character->currentTilePosition()))
        return;
    applyClock(ClockEvent::Tick);
    if (!m_transitionActive && !m_bedRecoveryPrompt && !m_morning)
        recoverToMorning(m_gameClock.elapsedGameMinutes(), false);
}

void MainWindow::requestSleep() {
    if (m_sleepConfirmation->isVisible() || m_transitionActive || m_panelOpen ||
        m_bedRecoveryPrompt || m_morning) return;
    m_sleepConfirmWasPaused = m_gameClock.isPaused();
    m_gameClock.pause();
    applyClock(ClockEvent::Tick);
    if (m_transitionActive || m_bedRecoveryPrompt || m_morning) {
        if (!m_sleepConfirmWasPaused && !m_bedRecoveryPrompt && !m_morning)
            m_gameClock.resume();
        return;
    }
    const auto target = GameClock::sleepTargetMinute(m_gameClock.elapsedGameMinutes());
    if (!target) {
        if (!m_sleepConfirmWasPaused) m_gameClock.resume();
        return;
    }
    const auto date = SeasonCalendar::dateForMinutes(*target);
    m_sleepConfirmation->present(QStringLiteral("第 %1 年 · %2 %3 日  06:00")
        .arg(date.year).arg(SeasonCalendar::seasonName(date.season)).arg(date.dayOfSeason));
    refreshGameplayInput();
}

void MainWindow::finishSleepConfirmation(bool confirmed) {
    if (!m_sleepConfirmation->isVisible() || m_closePrompt) return;
    m_sleepConfirmation->hide();
    if (confirmed) sleepUntilMorning();
    if (!m_sleepConfirmWasPaused && !m_morning && !m_bedRecoveryPrompt && m_page == Page::Farm)
        m_gameClock.resume();
    refreshGameplayInput();
}

std::optional<QPoint> MainWindow::bedWakePosition() const {
    const int tileWidth = mapWidget->tileWidth();
    const int tileHeight = mapWidget->tileHeight();
    // Try the walkable tile beside the foot of the bed first.
    const QPoint besideBed(Bed.right() + 1, Bed.bottom() + 1);
    for (int radius = 0; radius <= 4; ++radius) {
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                if (qMax(qAbs(dx), qAbs(dy)) != radius) continue;
                const QPoint tile = besideBed + QPoint(dx, dy);
                const QPoint position(tile.x() * tileWidth + (tileWidth - character->width()) / 2,
                                      tile.y() * tileHeight + (tileHeight - character->height()) / 2);
                if (character->canStandAt(position)) return position;
            }
        }
    }
    return std::nullopt;
}

bool MainWindow::recoverToMorning(qint64 from, bool passedOut) {
    const auto target = GameClock::sleepTargetMinute(from);
    if (!target) return false;
    if (passedOut) {
        const auto position = bedWakePosition();
        if (!position) {
            m_gameClock.pause();
            cancelStorageOpening();
            if (m_panelOpen) m_panelOpen->reject();
            auto *warning = new QMessageBox(QMessageBox::Warning, QStringLiteral("无法回到床边"),
                QStringLiteral("床边没有安全落点。农场已暂停，请重试。"), QMessageBox::NoButton, this);
            auto *retry = warning->addButton(QStringLiteral("重试"), QMessageBox::AcceptRole);
            retry->setObjectName(QStringLiteral("retryBedRecovery"));
            warning->setDefaultButton(retry);
            warning->setAttribute(Qt::WA_DeleteOnClose);
            m_bedRecoveryPrompt = warning;
            connect(warning, &QDialog::finished, this, [this, from] {
                m_bedRecoveryPrompt.clear();
                if (!m_closePrompt && m_page == Page::Farm) recoverToMorning(from, true);
            });
            refreshGameplayInput();
            warning->open();
            return false;
        }
        m_transitionActive = m_transition->start();
        cancelStorageOpening();
        if (m_panelOpen) m_panelOpen->reject();
        refreshGameplayInput();
        character->setInitialPosition(*position);
    }
    m_gameClock.setElapsedGameMinutes(*target);
    m_gameClock.pause();
    return applyClock(passedOut ? ClockEvent::PassOut : ClockEvent::WakeUp);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (m_page == Page::Farm && !m_transitionActive && !m_closePrompt &&
        m_wakeSaveState != WakeSaveState::Failed &&
        (event->type() == QEvent::KeyPress || event->type() == QEvent::MouseButtonPress)) {
        applyClock(ClockEvent::Tick);
        if (m_transitionActive || m_bedRecoveryPrompt) return true;
    }
    if (m_storageOpening && event->type() == QEvent::KeyPress) {
        const auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Escape && !key->isAutoRepeat()) {
            cancelStorageOpening();
            refreshGameplayInput();
        }
        return true;
    }
    if (m_page == Page::Farm && !m_panelOpen && !m_closePrompt &&
        m_wakeSaveState != WakeSaveState::Failed && event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Escape && key->modifiers() == Qt::NoModifier) {
            if (!key->isAutoRepeat()) pauseFarm();
            return true;
        }
    }
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

    // Expand the visible world to the screen aspect, rather than stretching the
    // original 4:3 scene or leaving large side bars on widescreen displays.
    const int logicalWidth = isFullScreen()
        ? qMax(320, qRound(qreal(CanvasHeight) * viewportSize.width() / viewportSize.height()))
        : CanvasWidth;
    const QSize canvasSize(logicalWidth, CanvasHeight);
    if (m_gameCanvas->size() != canvasSize)
        m_gameCanvas->setFixedSize(canvasSize);
    // Synchronize the proxy clip even if the widget already resized while hidden.
    if (auto *proxy = m_gameCanvas->graphicsProxyWidget()) {
        proxy->setMinimumSize(canvasSize);
        proxy->setMaximumSize(canvasSize);
        proxy->resize(canvasSize);
    }
    m_gameScene->setSceneRect(m_gameCanvas->rect());
    mapWidget->setGeometry(m_gameCanvas->rect());
    itemBar->move((logicalWidth - itemBar->width()) / 2, CanvasHeight - itemBar->height() - 8);
    m_hud->move(logicalWidth - m_hud->width() - OverlayMargin, OverlayMargin);
    positionStatus();
    character->setInitialPosition(character->absolutePosition());

    const qreal availableScale = qMin(
        static_cast<qreal>(viewportSize.width()) / m_gameCanvas->width(),
        static_cast<qreal>(viewportSize.height()) / m_gameCanvas->height());
    const qreal dpr = m_gameView->viewport()->devicePixelRatioF();
    const qreal pixelScale = !isFullScreen() && availableScale >= 1.0
        ? qFloor(availableScale * dpr) / dpr : availableScale;

    m_gameView->resetTransform();
    m_gameView->scale(pixelScale, pixelScale);
    m_gameView->centerOn(m_gameScene->sceneRect().center());
    m_gameView->viewport()->update();
}

void MainWindow::updateHud() {
    itemBar->setStock(m_economy);
    m_hud->setMoney(m_economy.coins());
    m_hud->setWater(m_economy.waterRemaining(), m_economy.waterCapacity());
    character->setAxeUpgraded(m_economy.axeUpgraded());
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
    positionStatus();
    m_statusLabel->show();
    m_statusTimer->start();
}

void MainWindow::positionStatus() {
    // Keep notifications in the space to the left of the corner HUD.
    const int available = m_gameCanvas->width() - m_hud->width() - 3 * OverlayMargin;
    m_statusLabel->setMaximumWidth(available);
    m_statusLabel->setWordWrap(false);
    m_statusLabel->setWordWrap(m_statusLabel->sizeHint().width() > available);
    m_statusLabel->adjustSize();
    m_statusLabel->move(OverlayMargin + (available - m_statusLabel->width()) / 2, OverlayMargin);
}

void MainWindow::openInventory() {
    if (m_page != Page::Farm || m_panelOpen || m_storageOpening || m_transitionActive || m_closePrompt ||
        m_wakeSaveState == WakeSaveState::Failed) {
        return;
    }
    auto *dialog = new InventoryDialog(this);
    connect(dialog, &InventoryDialog::slotRequested, this, [this, dialog](int slot) {
        if (!acceptPanelCommand(dialog)) return;
        itemBar->selectSlot(slot);
    });
    connect(dialog,&InventoryDialog::stackMoveRequested,this,[this,dialog](StackMove move){moveInventoryStack(dialog,move);});
    showFarmPanel(dialog);
}

void MainWindow::openSeedShop() {
    auto *dialog = new SeedShopDialog(this);
    const auto purchase = [this, dialog](ToolType tool, int quantity) {
        if (!acceptPanelCommand(dialog)) return;
        if (!stationReach(ShopCounter).contains(character->currentTilePosition())) {
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
    const auto upgrade = [this, dialog](ToolType tool) {
        if (!acceptPanelCommand(dialog)) return;
        if (!stationReach(ShopCounter).contains(character->currentTilePosition())) {
            dialog->setMessage(QStringLiteral("请到商店升级农具"));
            return;
        }
        const bool can = tool == ToolType::WateringCan;
        if (can ? m_economy.canUpgraded() : m_economy.axeUpgraded()) {
            dialog->setMessage(QStringLiteral("这件农具已升到最高级"));
            return;
        }
        if (!(can ? m_economy.upgradeCan() : m_economy.upgradeAxe())) {
            dialog->setMessage(QStringLiteral("金币不足"));
            return;
        }
        finishTrade(dialog, can ? QStringLiteral("水壶已升级并补满，花费 %1 金币").arg(EconomyService::CanUpgradePrice)
                                : QStringLiteral("斧头已升级，花费 %1 金币").arg(EconomyService::AxeUpgradePrice));
    };
    connect(dialog, &SeedShopDialog::canUpgradeRequested, this, [upgrade] { upgrade(ToolType::WateringCan); });
    connect(dialog, &SeedShopDialog::axeUpgradeRequested, this, [upgrade] { upgrade(ToolType::Axe); });
    showFarmPanel(dialog);
}

void MainWindow::openMarket() {
    auto *dialog = new MarketDialog(this);
    const auto showSale = [this, dialog](const SaleResult &sale, const QString &product) {
        if (!sale.success) {
            dialog->setMessage(QStringLiteral("未能出售，请检查站点、库存和金币余额"));
            return;
        }
        finishTrade(dialog, QStringLiteral("已售出 %1 份%2，收入 %3 金币")
                                .arg(sale.itemsSold).arg(product).arg(sale.coinsEarned));
    };
    connect(dialog, &MarketDialog::cropSaleRequested, this, [this, dialog, showSale](CropType crop, int quantity) {
        if (!acceptPanelCommand(dialog)) return;
        showSale(m_economy.sellCrop(crop, quantity, character->currentTilePosition(), stationReach(MarketCounter)),
                 CropCatalog::definition(crop).localizedName);
    });
    connect(dialog, &MarketDialog::saleRequested, this, [this, dialog, showSale] {
        if (!acceptPanelCommand(dialog)) return;
        showSale(m_economy.sellAll(character->currentTilePosition(), stationReach(MarketCounter)), QStringLiteral("农产品"));
    });
    showFarmPanel(dialog);
}

void MainWindow::openStorage() {
    if (m_page != Page::Farm || m_panelOpen || m_storageOpening || m_transitionActive || m_closePrompt ||
        m_wakeSaveState == WakeSaveState::Failed ||
        !stationReach(StorageChest).contains(character->currentTilePosition())) return;
    applyClock(ClockEvent::Tick);
    if (m_transitionActive || m_wakeSaveState == WakeSaveState::Failed ||
        !stationReach(StorageChest).contains(character->currentTilePosition())) return;
    m_storageOpening = true;
    refreshGameplayInput();
    mapWidget->setStorageChestOpen(true);
}

void MainWindow::cancelStorageOpening() {
    if (!m_storageOpening) return;
    m_storageOpening = false;
    mapWidget->setStorageChestOpen(false);
}

void MainWindow::finishStorageOpening() {
    if (!m_storageOpening) return;
    applyClock(ClockEvent::Tick);
    if (!m_storageOpening) return;
    m_storageOpening = false;
    if (m_page != Page::Farm || m_panelOpen || m_transitionActive || m_closePrompt ||
        m_wakeSaveState == WakeSaveState::Failed ||
        !stationReach(StorageChest).contains(character->currentTilePosition())) {
        mapWidget->setStorageChestOpen(false);
        refreshGameplayInput();
        return;
    }
    auto *dialog = new StorageDialog(this);
    connect(dialog, &QDialog::finished, mapWidget, [this] { mapWidget->setStorageChestOpen(false); });
    connect(dialog,&StorageDialog::stackMoveRequested,this,[this,dialog](StackMove move){moveInventoryStack(dialog,move);});
    connect(dialog, &StorageDialog::transferRequested, this,
            [this, dialog](StorageItemKind kind, CropType crop, StorageDirection direction, int quantity) {
        if (!acceptPanelCommand(dialog)) return;
        if (!stationReach(StorageChest).contains(character->currentTilePosition())) {
            dialog->setMessage(QStringLiteral("请靠近储物箱存取"));
            return;
        }
        const auto result = m_economy.transfer(kind, crop, direction, quantity);
        if (result.success)
            finishTrade(dialog, result.message);
        else
            dialog->setMessage(result.message);
    });
    showFarmPanel(dialog);
    if (m_panelOpen != dialog) mapWidget->setStorageChestOpen(false);
}

void MainWindow::finishTrade(FarmPanel *dialog, const QString &receipt) {
    updateHud();
    refreshOpenPanel();
    const QString message = saveGame(false) ? receipt : receipt + QStringLiteral("（未保存）");
    dialog->setMessage(message);
    showStatus(message);
}

void MainWindow::showFarmPanel(QDialog *dialog) {
    if (m_page != Page::Farm || m_panelOpen || m_storageOpening || m_transitionActive || m_closePrompt ||
        m_sleepConfirmation->isVisible() || m_morning || m_wakeSaveState == WakeSaveState::Failed) {
        dialog->deleteLater();
        return;
    }
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
            if (isVisible() && m_page == Page::Farm && !m_panelOpen && !m_transitionActive) {
                activateWindow();
                refreshGameplayInput();
            }
        });
    });
    refreshOpenPanel();
    refreshGameplayInput();
    dialog->open();
}

bool MainWindow::acceptPanelCommand(QDialog *panel) {
    if (m_panelOpen != panel || m_page != Page::Farm || m_transitionActive || m_closePrompt ||
        m_wakeSaveState == WakeSaveState::Failed) return false;
    applyClock(ClockEvent::Tick);
    return m_panelOpen == panel && !m_transitionActive &&
           m_wakeSaveState != WakeSaveState::Failed;
}

void MainWindow::moveInventoryStack(QDialog *panel,const StackMove &move) {
    if (panel) {
        if (!acceptPanelCommand(panel)) return;
    } else {
        if (m_page!=Page::Farm || m_panelOpen || m_storageOpening || m_transitionActive || m_closePrompt ||
            m_wakeSaveState==WakeSaveState::Failed) return;
        applyClock(ClockEvent::Tick);
        if (m_transitionActive || m_panelOpen) return;
    }
    if ((move.sourceChest || move.targetChest) &&
        (!qobject_cast<StorageDialog *>(panel) || !stationReach(StorageChest).contains(character->currentTilePosition()))) return;
    if (!m_economy.moveStack(move)) {
        QToolTip::showText(QCursor::pos(),QStringLiteral("无法移动：目标、数量或库存已变化，物品已保留"),this);
        return;
    }
    updateHud(); refreshOpenPanel(); saveGame(false);
}

void MainWindow::refreshOpenPanel() {
    if (auto *bag = qobject_cast<InventoryDialog *>(m_panelOpen.data())) {
        bag->setState(m_economy, itemBar->selectedTool(), currentGameDay(),itemBar->selectedSlot());
    } else if (auto *shop = qobject_cast<SeedShopDialog *>(m_panelOpen.data())) {
        shop->setState(m_economy);
    } else if (auto *market = qobject_cast<MarketDialog *>(m_panelOpen.data())) {
        market->setState(m_economy);
    } else if (auto *storage = qobject_cast<StorageDialog *>(m_panelOpen.data())) {
        storage->setState(m_economy);
    }
}

void MainWindow::refreshGameplayInput() {
    if (m_storageOpening && (m_page != Page::Farm || m_transitionActive || m_closePrompt ||
        m_wakeSaveState == WakeSaveState::Failed)) cancelStorageOpening();
    const bool blocked = m_page != Page::Farm || m_panelOpen || m_storageOpening || m_transitionActive || m_closePrompt ||
                         m_sleepConfirmation->isVisible() || m_morning || m_bedRecoveryPrompt ||
                         m_wakeSaveState == WakeSaveState::Failed;
    character->setInputBlocked(blocked);
    mapWidget->setInputBlocked(blocked);
    itemBar->setEnabled(!blocked);
    if (m_panelOpen) {
        m_panelOpen->setEnabled(!m_transitionActive && !m_closePrompt);
    }
    if (!blocked) {
        m_gameView->setFocus(Qt::OtherFocusReason);
        character->setFocus(Qt::OtherFocusReason);
    }
}

void MainWindow::finishWakeSave() {
    if (m_transitionActive || m_closePrompt || m_page != Page::Farm || !m_morning || !isVisible()) return;
    const bool failed = m_wakeSaveState == WakeSaveState::Failed;
    if (m_pages->currentWidget() == m_wakeSummary) {
        if (failed) showWakeSaveFailure();
        else m_wakeSummary->focusContinue();
        return;
    }
    const auto dateText = [](int day) {
        const auto date = SeasonCalendar::dateForDay(day);
        return QStringLiteral("第 %1 年 · %2 %3 日")
            .arg(date.year).arg(SeasonCalendar::seasonName(date.season)).arg(date.dayOfSeason);
    };
    const auto &morning = *m_morning;
    const QString date = dateText(currentGameDay());
    const QString dateChange = morning.previousDay == currentGameDay()
        ? QStringLiteral("同日休息至早晨")
        : dateText(morning.previousDay) + QStringLiteral("  ›  ") + date;
    const QStringList rows{
        QStringLiteral("成长作物：%1 株").arg(morning.result.cropsAdvanced),
        QStringLiteral("缺水停滞：%1 株").arg(morning.result.cropsStalled),
        QStringLiteral("恢复的空耕地：%1 格").arg(morning.result.soilRecovered)};
    const bool saved = m_wakeSaveState == WakeSaveState::Saved;
    QString saveStatus = QStringLiteral("只读农场，进度未保存");
    if (failed) saveStatus = QStringLiteral("保存失败，进度未保存");
    else if (saved) saveStatus = QStringLiteral("进度已保存");
    m_wakeSummary->present(morning.passedOut ? QStringLiteral("凌晨昏倒，在床边醒来")
                                            : QStringLiteral("新的一天"),
                           date + QStringLiteral("  06:00"), dateChange, rows,
                           saveStatus, saved, !failed);
    m_pages->setCurrentWidget(m_wakeSummary);
    if (failed) {
        showWakeSaveFailure();
        return;
    }
    m_wakeSaveState = WakeSaveState::None;
    m_wakeSummary->focusContinue();
    if (saved) emit wakeSaved();
}

void MainWindow::leaveMorningPage() {
    if (m_pages->currentWidget() != m_wakeSummary || m_closePrompt ||
        m_wakeSaveState == WakeSaveState::Failed) return;
    m_morning.reset();
    m_pages->setCurrentWidget(m_gameView);
    m_gameClock.resume();
    refreshGameplayInput();
}

void MainWindow::showWakeSaveFailure() {
    if (m_wakeSaveState != WakeSaveState::Failed || m_transitionActive || m_closePrompt) return;
    if (m_wakeSavePrompt) {
        m_wakeSavePrompt->show();
        m_wakeSavePrompt->raise();
        return;
    }
    auto *question = new QMessageBox(QMessageBox::Warning, QStringLiteral("睡醒后保存失败"),
        QStringLiteral("进度还没保存。可以检查存档目录后重试，也可以继续游戏。"),
        QMessageBox::NoButton, this);
    question->setObjectName(QStringLiteral("wakeSaveFailure"));
    question->setAttribute(Qt::WA_DeleteOnClose);
    auto *retry = question->addButton(QStringLiteral("重试保存"), QMessageBox::AcceptRole);
    auto *keepPlaying = question->addButton(QStringLiteral("继续游戏"), QMessageBox::RejectRole);
    retry->setObjectName(QStringLiteral("retryWakeSave"));
    keepPlaying->setObjectName(QStringLiteral("continueAfterWakeSave"));
    question->setDefaultButton(retry);
    question->setEscapeButton(keepPlaying);
    m_wakeSavePrompt = question;
    connect(question, &QDialog::finished, this, [this, question, retry] {
        m_wakeSavePrompt.clear();
        if (question->clickedButton() == retry) {
            // Retry the settled snapshot only: never advance the clock or crops again.
            if (!writeGameSave(false)) {
                QTimer::singleShot(0, this, &MainWindow::showWakeSaveFailure);
                return;
            }
            resolveWakeSaveFailure(true);
        } else {
            resolveWakeSaveFailure(false);
        }
    });
    question->open();
}

void MainWindow::resolveWakeSaveFailure(bool saved) {
    m_wakeSaveState = WakeSaveState::None;
    leaveMorningPage();
    showStatus(saved ? QStringLiteral("早晨进度已保存。") : QStringLiteral("进度尚未保存，已继续游戏。"));
    m_tick->start();
    m_autosave->start();
    m_music->play();
    refreshGameplayInput();
    if (saved) emit wakeSaved();
    // Restore focus after the modal dialog has completed its dismissal.
    QTimer::singleShot(0, this, [this] {
        if (isVisible() && m_page == Page::Farm && !m_closePrompt &&
            m_wakeSaveState == WakeSaveState::None) {
            activateWindow();
            refreshGameplayInput();
        }
    });
}

bool MainWindow::saveGame(bool showError) {
    // World actions and background saves cannot resolve a failed wake implicitly.
    if (m_bedRecoveryPrompt || m_wakeSaveState == WakeSaveState::Failed) return false;
    return writeGameSave(showError);
}

bool MainWindow::writeGameSave(bool showError) {
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

bool MainWindow::readFarmSave(const QString &source, GameSaveData *data, QString *loadedPath) const {
    if (source.isEmpty()) return false;
    for (const QString &path : {source, source + QStringLiteral(".bak")}) {
        if (SaveManager::load(path, data) && isCompatibleSave(*data)) {
            *loadedPath = path;
            return true;
        }
    }
    return false;
}

void MainWindow::loadGame() {
    const QString source = SaveManager::preferredSavePath();
    if (source.isEmpty()) {
        showStatus(QStringLiteral("WASD 移动，滚轮选工具。E / Tab 背包，F 互动，Esc 暂停。"));
        return;
    }
    GameSaveData data;
    QString path;
    if (readFarmSave(source, &data, &path) && restoreGame(data)) {
        showStatus(path == source ? QStringLiteral("已读取农场存档")
                                  : QStringLiteral("已从备份恢复农场"));
        return;
    }
    if (source == SaveManager::defaultSavePath()) {
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
    return mapWidget->canRestoreWorld(data.world) && !layer.isEmpty() && layer.size() == mapWidget->mapHeight() &&
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
    if (!candidate.restore(data.economy) || !mapWidget->restoreWorld(data.world) ||
        !character->restorePosition(data.playerPosition)) {
        return false;
    }
    m_economy = std::move(candidate);
    m_gameClock.setElapsedGameMinutes(data.gameMinutes);
    m_lastProcessedDay = data.lastProcessedDay;
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
    cancelStorageOpening();
    if (m_page == Page::Home || !m_saveEnabled || m_discardOnClose) {
        m_discardOnClose = false;
        m_wakeSaveState = WakeSaveState::None;
        QMainWindow::closeEvent(event);
        return;
    }
    if (m_closePrompt) {
        event->ignore();
        m_closePrompt->raise();
        return;
    }

    m_gameClock.pause();
    m_tick->stop();
    m_autosave->stop();
    // Sample the frozen minute and settle any midnight before the single write attempt.
    const bool saved = m_page == Page::Farm ? applyClock(ClockEvent::Save) : saveGame(false);
    if (saved) {
        m_wakeSaveState = WakeSaveState::None;
        QMainWindow::closeEvent(event);
        return;
    }

    event->ignore();
    m_music->pause();
    if (m_wakeSavePrompt) m_wakeSavePrompt->hide();
    const QPointer<QWidget> focus = QApplication::focusWidget();
    auto *question = new QMessageBox(QMessageBox::Warning, QStringLiteral("退出前保存失败"),
        QStringLiteral("农场未能保存。可以留下检查存档目录，也可以放弃上次保存之后的进度并退出。"),
        QMessageBox::NoButton, this);
    question->setObjectName(QStringLiteral("closeSaveFailure"));
    question->setAttribute(Qt::WA_DeleteOnClose);
    auto *stay = question->addButton(QStringLiteral("留在农场"), QMessageBox::RejectRole);
    auto *discard = question->addButton(QStringLiteral("放弃未保存进度并退出"), QMessageBox::DestructiveRole);
    stay->setObjectName(QStringLiteral("keepFarmOpen"));
    discard->setObjectName(QStringLiteral("discardAndQuit"));
    question->setDefaultButton(stay);
    question->setEscapeButton(stay);
    m_closePrompt = question;
    refreshGameplayInput();
    connect(question, &QDialog::finished, this, [this, question, discard, focus] {
        m_closePrompt.clear();
        if (question->clickedButton() == discard) {
            m_discardOnClose = true;
            close();
            return;
        }
        if (m_page == Page::Farm && m_wakeSaveState != WakeSaveState::Failed) {
            if (!m_morning && !m_sleepConfirmation->isVisible() && !m_bedRecoveryPrompt)
                m_gameClock.resume();
            m_tick->start();
            m_autosave->start();
            m_music->play();
        }
        refreshGameplayInput();
        finishWakeSave();
        if (focus && focus->isVisible() && focus->isEnabled()) focus->setFocus(Qt::OtherFocusReason);
    });
    question->open();
}
