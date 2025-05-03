//mainwinow.cpp
#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QFontDatabase>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , mapWidget(new MapWidget(this))
    , overlay(new QWidget(this))
    , clockBgWidget(new ClockBgWidget(this))
    ,timeLabel(new QLabel(overlay))
    ,dayLabel(new QLabel(overlay))
{

    m_gameTimer.start(); // 初始化计时器
    // ==================== 窗口基础设置 ====================

    setFixedSize(520, 320);

    // 地图控件尺寸与主窗口一致
    mapWidget->setFixedSize(520, 320);
    mapWidget->loadMap(":/map/map6.tmj");
    mapWidget->lower();

    // ==================== 时钟背景层设置 ====================
    clockBgWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    clockBgWidget->setFixedSize(520, 320);
    clockBgWidget->raise(); // 关键修改：将覆盖层置底

    // ==================== 覆盖层设置 ====================
    overlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    overlay->setFixedSize(520, 320);
    overlay->raise(); // 关键修改：将覆盖层置底


    // ==================== 角色控件初始化 ====================
    character = new ImageMovementWidget(mapWidget);
    // 初始位置设置为地图中心（根据地图尺寸动态计算）
    QPoint initialPos(
        mapWidget->tileWidth() * mapWidget->mapWidth() / 2,
        mapWidget->tileHeight() * mapWidget->mapHeight() / 2
        );
    character->setInitialPosition(initialPos);



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


    connect(mapWidget, &MapWidget::mapClicked, [this](QPoint pos) {
        // 通过character获取当前工具
        const ToolProperty &tool = character->getCurrentTool();
        qDebug() << "=== 鼠标点击事件 ===";
        qDebug() << "点击位置:" << pos << "当前工具:" << tool.name;
        character->handleToolAction();
    });

    // 连接物品栏的索引变化信号到角色工具切换
    connect(itemBar, &ItemBar::itemSelected,
            character, &ImageMovementWidget::setCurrentTool);

    connect(mapWidget, &MapWidget::mapClicked, character, [this](QPoint pos){
        if (character->getCurrentTool().type == ToolType::Hoe) {
            character->useHoe(pos);
        } else if (character->getCurrentTool().type == ToolType::Wheat) {
            character->useWheat(pos);
        }
    });




    // 加载字体和背景
    loadCustomFont();
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
        return qMakePair(currentGameDay(), currentGameHour());
    });
}



bool MainWindow::loadCustomFont() {
    int fontId = QFontDatabase::addApplicationFont(":/tilesets/sprout/pixelFont-7-8x14-sproutLands.ttf");
    if (fontId == -1) return false;
    QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    if (families.isEmpty()) return false;
    digitalFont = QFont(families.at(0), 20); // 调整字体大小
    return true;
}

void ClockBgWidget::paintEvent(QPaintEvent *event)  {
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

// ==================== 程序入口 ====================
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);


    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
