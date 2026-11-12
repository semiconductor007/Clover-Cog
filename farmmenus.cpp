#include "farmmenus.h"

#include <QFrame>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {
QPushButton *button(const QString &label, const QString &name, bool primary = false) {
    auto *result = new QPushButton(label);
    result->setObjectName(name);
    result->setProperty("primary", primary);
    result->setCursor(Qt::PointingHandCursor);
    result->setMinimumHeight(34);
    return result;
}

QLabel *heading(const QString &text) {
    auto *label = new QLabel(text);
    label->setObjectName("menuHeading");
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    return label;
}

QLabel *noticeLabel(const QString &name) {
    auto *label = new QLabel;
    label->setObjectName(name);
    label->setWordWrap(true);
    label->setProperty("notice", true);
    return label;
}

// Share the farm's actual ground pixels between the guide and the map.
QWidget *sample(const QString &title, const QString &file, const QRect &cell, bool grassBase = false) {
    const QPixmap sheet(file);
    QPixmap picture(48, 48);
    picture.fill(Qt::transparent);
    QPainter painter(&picture);
    if (grassBase) {
        const QPixmap grass(QStringLiteral(":/tilesets/sprout/Grass.png"));
        painter.drawPixmap(picture.rect(), grass, QRect(16, 16, 16, 16));
    }
    painter.drawPixmap(picture.rect(), sheet, cell.isEmpty() ? sheet.rect() : cell);
    painter.end();
    auto *sample = new QWidget;
    auto *row = new QHBoxLayout(sample);
    row->setContentsMargins(0, 0, 0, 0);
    auto *art = new QLabel;
    art->setPixmap(picture);
    row->addWidget(art);
    row->addWidget(new QLabel(title));
    row->addStretch();
    return sample;
}

class Landscape : public QWidget {
public:
    explicit Landscape(const QPixmap &farm) : m_farm(farm) {
        setMinimumHeight(70);
        setMaximumHeight(245);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
    QSize sizeHint() const override { return {540, 205}; }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        const auto target = rect().adjusted(2, 2, -2, -2);
        const QSize source = target.size().scaled(m_farm.size(), Qt::KeepAspectRatio);
        QRect crop(QPoint(), source);
        crop.moveCenter(m_farm.rect().center());
        painter.fillRect(rect(), QColor("#765b3e"));
        painter.drawPixmap(target, m_farm, crop);
    }
private:
    QPixmap m_farm;
};
}

FarmMenus::FarmMenus(const QPixmap &farm, QWidget *parent)
    : QStackedWidget(parent), m_home(new QWidget), m_pause(new QWidget), m_guide(new QWidget) {
    setObjectName("farmMenus");
    setStyleSheet(QStringLiteral(
        "QStackedWidget#farmMenus { background: #304b3d; }"
        "QFrame#menuCard { background: #f3e5be; border: 2px solid #9c7d51; }"
        "QLabel { color: #3a3629; font-size: 14px; } QLabel#menuHeading { color: #294a36; font-size: 28px; font-weight: bold; }"
        "QLabel[notice=true] { color: #7e3e21; }"
        "QPushButton { background: #e6cfa2; color: #423527; border: 1px solid #93744e;"
        " border-bottom: 3px solid #866847; padding: 3px 14px; }"
        "QPushButton[primary=true] { background: #426747; color: #fff5d7; border-color: #304c36; }"
        "QPushButton:hover { background: #d5bd90; } QPushButton[primary=true]:hover { background: #507a52; }"
        "QPushButton:focus { border: 2px solid #975c26; } QPushButton:pressed { border-bottom-width: 1px; }"));
    const auto card = [this](QWidget *page, const QString &name) {
        addWidget(page);
        page->setObjectName(name);
        auto *outer = new QVBoxLayout(page);
        outer->setContentsMargins(12, 12, 12, 12);
        outer->setAlignment(Qt::AlignCenter);
        auto *frame = new QFrame;
        frame->setObjectName("menuCard");
        frame->setMaximumWidth(640);
        frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        outer->addWidget(frame);
        auto *layout = new QVBoxLayout(frame);
        layout->setContentsMargins(17, 14, 17, 14);
        layout->setSpacing(9);
        return layout;
    };

    auto *home = card(m_home, "homePage");
    home->parentWidget()->setMaximumHeight(360);
    home->addWidget(heading(QStringLiteral("四季农场")));
    home->addWidget(new Landscape(farm));
    m_homeNotice = noticeLabel("homeNotice");
    home->addWidget(m_homeNotice);
    m_enter = button(QStringLiteral("开始种田"), "enterFarm", true);
    m_homeGuide = button(QStringLiteral("玩法说明"), "homeGuide");
    auto *exit = button(QStringLiteral("退出"), "exitGame");
    auto *homeButtons = new QHBoxLayout;
    homeButtons->addWidget(m_enter, 2);
    homeButtons->addWidget(m_homeGuide, 1);
    homeButtons->addWidget(exit);
    home->addLayout(homeButtons);
    connect(m_enter, &QPushButton::clicked, this, &FarmMenus::enterRequested);
    connect(m_homeGuide, &QPushButton::clicked, this, &FarmMenus::showGuide);
    connect(exit, &QPushButton::clicked, this, &FarmMenus::exitRequested);

    auto *pause = card(m_pause, "pausePage");
    pause->parentWidget()->setMaximumWidth(410);
    pause->addWidget(heading(QStringLiteral("游戏暂停")));
    m_pauseNotice = noticeLabel("pauseNotice");
    pause->addWidget(m_pauseNotice);
    m_resume = button(QStringLiteral("继续游戏"), "resumeFarm", true);
    m_pauseGuide = button(QStringLiteral("玩法说明"), "pauseGuide");
    auto *leave = button(QStringLiteral("保存并返回首页"), "saveReturnHome");
    pause->addWidget(m_resume);
    pause->addWidget(m_pauseGuide);
    pause->addWidget(leave);
    connect(m_resume, &QPushButton::clicked, this, &FarmMenus::resumeRequested);
    connect(m_pauseGuide, &QPushButton::clicked, this, &FarmMenus::showGuide);
    connect(leave, &QPushButton::clicked, this, &FarmMenus::saveAndLeaveRequested);

    auto *guide = card(m_guide, "guidePage");
    guide->addWidget(heading(QStringLiteral("玩法说明")));
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea, QWidget#guideContent { background: #f3e5be; }");
    auto *content = new QWidget;
    content->setObjectName("guideContent");
    auto *body = new QVBoxLayout(content);
    body->setContentsMargins(0, 0, 4, 0);
    auto *samples = new QHBoxLayout;
    samples->addWidget(sample(QStringLiteral("草地可翻"), ":/tilesets/sprout/Grass.png", {0,0,48,48}));
    samples->addWidget(sample(QStringLiteral("耕地可种"), ":/tilesets/sprout/Tilled_Dirt_Wide_v2.png", {0,0,48,48}, true));
    samples->addWidget(sample(QStringLiteral("石路通行"), ":/tilesets/sprout/Stone_Path.png", {0,0,16,16}, true));
    body->addLayout(samples);
    auto *tools = new QHBoxLayout;
    const QString icons = QStringLiteral(":/tilesets/sprout/Basic_tools_and_meterials.png");
    tools->addWidget(sample(QStringLiteral("空手收获"), ":/ui/sprout/Catpaw Mouse icon.png", {}));
    tools->addWidget(sample(QStringLiteral("锄头翻土"), icons, {32,0,16,16}));
    tools->addWidget(sample(QStringLiteral("水壶浇地"), icons, {0,0,16,16}));
    body->addLayout(tools);
    auto *route = new QLabel(QStringLiteral(
        "选锄头点草地，或直接找到已耕的田。选当季种子点田播种，换水壶浇水。每过一个浇过水的夜晚，作物就会生长。\n\n"
        "空手点成熟作物即可收获。走到收购摊按 E 出售；在储物箱南侧按 E 存取物品。种子和肥料可以在商店购买。\n\n"
        "WASD 移动　·　滚轮或点击工具格选物　·　左键使用\n"
        "E 站点互动　·　Tab 背包　·　Esc 暂停或关闭面板"));
    route->setWordWrap(true);
    body->addWidget(route);
    body->addStretch();
    scroll->setWidget(content);
    scroll->setMinimumHeight(155);
    guide->addWidget(scroll);
    m_back = button(QStringLiteral("返回"), "guideBack");
    guide->addWidget(m_back);
    connect(m_back, &QPushButton::clicked, this, &FarmMenus::leaveGuide);
}

void FarmMenus::showHome(bool canContinue, const QString &notice) {
    m_enter->setText(canContinue ? QStringLiteral("继续农场") : QStringLiteral("开始种田"));
    m_homeNotice->setText(notice);
    m_homeNotice->setVisible(!notice.isEmpty());
    setCurrentWidget(m_home);
    m_enter->setFocus();
}

void FarmMenus::showPause(const QString &notice) {
    m_pauseNotice->setText(notice);
    m_pauseNotice->setVisible(!notice.isEmpty());
    setCurrentWidget(m_pause);
    m_resume->setFocus();
}

void FarmMenus::showGuide() {
    m_guideOrigin = currentWidget();
    setCurrentWidget(m_guide);
    m_back->setFocus();
}

void FarmMenus::leaveGuide() {
    setCurrentWidget(m_guideOrigin);
    (m_guideOrigin == m_home ? m_homeGuide : m_pauseGuide)->setFocus();
}

void FarmMenus::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        if (!event->isAutoRepeat()) {
            if (currentWidget() == m_guide) leaveGuide();
            else if (currentWidget() == m_pause) emit resumeRequested();
        }
        event->accept();
        return;
    }
    QStackedWidget::keyPressEvent(event);
}
