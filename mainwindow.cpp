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
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QUrl>
#include <QWheelEvent>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_gameClock(GameConstants::RealMillisecondsPerGameMinute)
    , m_economy(GameConstants::InitialInventoryCapacity)
    , mapWidget(new MapWidget(this))
    , overlay(new QWidget(this))
    , clockBgWidget(new ClockBgWidget(this))
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


    m_gameClock.start();
    // ==================== 窗口基础设置 ====================

    resize(520, 320);

    // 地图控件尺寸与主窗口一致
    mapWidget->setFixedSize(520, 320);
    if (!mapWidget->loadMap(":/map/map6.tmj")) {
        qCritical() << "Unable to initialize the game map";
    }
    mapWidget->lower();
    mapWidget->setMarketArea(m_marketArea);
    mapWidget->setShopArea(m_shopArea);
    mapWidget->setBedArea(m_bedArea);

    // ==================== 时钟背景层设置 ====================
    clockBgWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    clockBgWidget->setFixedSize(520, 320);
    clockBgWidget->raise(); // 关键修改：将覆盖层置底

    // ==================== 覆盖层设置 ====================
    overlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    overlay->setFixedSize(520, 320);
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
    QPoint initialPos(250, 170);
    character->setInitialPosition(initialPos);
    // 初始位置设置为地图中心（根据地图尺寸动态计算）
    // QPoint initialPos(500, 200);
    // character->setInitialPosition(initialPos);



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
                if (m_shopArea.contains(playerTile)) {
                    openSeedShop();
                    return;
                }
                if (m_bedArea.contains(playerTile)) {
                    sleepUntilMorning();
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


    // 为窗口安装事件过滤器
    this->installEventFilter(this);

    // 将时间系统传递给mapWidget（需要先在MapWidget类中添加接收方法）
    mapWidget->setTimeReference([this]() {
        return m_gameClock.elapsedGameMinutes();
    });
    const QString savePath = SaveManager::defaultSavePath();
    const bool hadSave = QFileInfo::exists(savePath) ||
                         QFileInfo::exists(savePath + ".bak");
    const bool loadedSave = loadGame();
    updateHud();
    if (loadedSave) {
        showStatus(QStringLiteral("SAVE LOADED"));
    } else if (!hadSave) {
        showStatus(QStringLiteral("SELL  |  SEEDS  |  SLEEP - PRESS E"));
    }

    QTimer *autosaveTimer = new QTimer(this);
    connect(autosaveTimer, &QTimer::timeout,
            this, [this]() { saveGame(false); });
    autosaveTimer->start(60000);
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
    processDayTransitions();
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
        saveGame(false);
    }
    return total;
}

void MainWindow::sleepUntilMorning() {
    m_gameClock.sleepToNextMorning(GameConstants::WakeUpHour);
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
    SeedShopDialog dialog(this);
    const auto refreshDialog = [this, &dialog]() {
        dialog.setState(m_economy.coins(),
                        m_economy.seedCount(CropType::Wheat),
                        m_economy.seedCount(CropType::Potato));
    };
    refreshDialog();

    connect(&dialog, &SeedShopDialog::purchaseRequested,
            this, [this, &dialog, refreshDialog](CropType type, int quantity) {
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
    character->setFocus();
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
    if (!SaveManager::save(SaveManager::defaultSavePath(), data,
                           &errorMessage)) {
        if (showError) {
            showStatus(QStringLiteral("SAVE FAILED: %1").arg(errorMessage));
        }
        return false;
    }
    return true;
}

bool MainWindow::loadGame() {
    const QString savePath = SaveManager::defaultSavePath();
    if (!QFileInfo::exists(savePath) && !QFileInfo::exists(savePath + ".bak")) {
        return false;
    }

    GameSaveData data;
    QString errorMessage;
    bool loaded = SaveManager::load(savePath, &data, &errorMessage);
    if (!loaded) {
        loaded = SaveManager::load(savePath + ".bak", &data, &errorMessage);
    }
    const int currentDay = static_cast<int>(data.gameMinutes / (24 * 60));
    const int mapPixelWidth = mapWidget->mapWidth() * mapWidget->tileWidth();
    const int mapPixelHeight = mapWidget->mapHeight() * mapWidget->tileHeight();
    if (!loaded || data.lastProcessedDay > currentDay ||
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
    m_lastProcessedDay = data.lastProcessedDay;
    character->setInitialPosition(data.playerPosition);
    return true;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
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
    saveGame(false);
    QMainWindow::closeEvent(event);
}

// ==================== 程序入口 ====================
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("CloverCog"));
    QApplication::setApplicationName(QStringLiteral("Clover-Cog"));


    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
