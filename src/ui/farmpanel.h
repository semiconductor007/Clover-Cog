#ifndef FARMPANEL_H
#define FARMPANEL_H

#include <QDialog>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

// Shared chrome for the three farm panels. All quantities come from the caller.
class FarmPanel : public QDialog {
public:
    void setMessage(const QString &message) { m_message->setText(message); }

protected:
    void useTradeStyle() {
        m_tradeStyle = true;
        setWindowFlag(Qt::FramelessWindowHint);
        body->setContentsMargins(22, 18, 22, 18);
        body->setSpacing(14);
        setStyleSheet(QStringLiteral(
            "QDialog { background: #e8cfa6; }"
            "QLabel { color: #563b2b; font-size: 14px; background: transparent; }"
            "QLabel#panelTitle { font-size: 22px; font-weight: bold; color: #674532; }"
            "QLabel#panelSummary { font-weight: bold; background: #d2aa7e; padding: 7px 12px; border: 2px solid #a37754; }"
            "QLabel#tableHeading { color: #8c6247; font-size: 12px; padding-bottom: 6px; border-bottom: 2px solid #b58b65; }"
            "QLabel#panelMessage { font-size: 12px; color: #79543b; min-height: 32px; }"
            "QLabel#saleTotal { font-weight: bold; padding: 8px; background: #dbc099; }"
            "QPushButton { color: #563b2b; background: #f5dfb7; border: 2px solid #a37754; border-bottom: 4px solid #8c6247; padding: 6px 12px; font-size: 13px; }"
            "QPushButton:hover { background: #ffeac4; }"
            "QPushButton:focus { border-color: #674532; }"
            "QPushButton:pressed { background: #c69c72; border-bottom-width: 2px; }"
            "QPushButton:disabled { color: #a08c73; background: #dcc8a7; border-color: #bfa583; }"
            "QPushButton#closePanel { font-size: 20px; padding: 0; }"
            "QSpinBox { color: #563b2b; background: #fff0d1; border: 2px solid #a37754; padding: 5px; font-size: 14px; min-width: 70px; }"
            "QSpinBox:disabled { color: #a08c73; background: #dcc8a7; }"));
        auto *close = findChild<QPushButton *>(QStringLiteral("closePanel"));
        body->itemAt(body->count()-1)->layout()->removeWidget(close);
        close->setText(QStringLiteral("×"));
        close->setFixedSize(30,30);
        close->setAccessibleName(QStringLiteral("关闭面板"));
        close->setToolTip(QStringLiteral("关闭（Esc）"));
        static_cast<QHBoxLayout *>(body->itemAt(0)->layout())->addWidget(close);
        setMinimumWidth(620);
    }

    void paintEvent(QPaintEvent *event) override {
        if (!m_tradeStyle) { QDialog::paintEvent(event); return; }
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        const QPixmap frame = QPixmap(QStringLiteral(":/farm-ui/slots.png")).copy(2,98,44,44);
        const int source[] = {0,7,37,44};
        const int xs[] = {0,10,width()-10,width()};
        const int ys[] = {0,10,height()-10,height()};
        for (int y=0;y<3;++y) for(int x=0;x<3;++x)
            painter.drawPixmap(QRect(xs[x],ys[y],xs[x+1]-xs[x],ys[y+1]-ys[y]),frame,
                QRect(source[x],source[y],source[x+1]-source[x],source[y+1]-source[y]));
        for (auto *image : findChildren<QLabel *>()) {
            if (!image->property("tradeRow").isValid()) continue;
            const int row = image->property("tradeRow").toInt();
            const QRect band(16,image->y()-4,width()-32,image->height()+8);
            painter.fillRect(band, row%2 ? QColor("#efd9b3") : QColor("#dfbf94"));
            painter.setPen(QColor("#c5a077"));
            painter.drawLine(band.bottomLeft(),band.bottomRight());
        }
    }
    FarmPanel(const QString &title, QWidget *parent)
        : QDialog(parent), body(new QVBoxLayout(this)), summary(new QLabel(this)) {
        setWindowTitle(title);
        setModal(true);
        body->setContentsMargins(18, 14, 18, 14);
        body->setSpacing(10);
        auto *heading = new QHBoxLayout;
        auto *name = new QLabel(title, this);
        name->setObjectName(QStringLiteral("panelTitle"));
        summary->setObjectName(QStringLiteral("panelSummary"));
        summary->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        heading->addWidget(name);
        heading->addStretch();
        heading->addWidget(summary);
        body->addLayout(heading);

        setStyleSheet(QStringLiteral(
            "QDialog { background: #efe2bc; }"
            "QLabel { color: #3e3328; }"
            "QLabel#panelTitle { color: #284736; font-size: 20px; font-weight: bold; }"
            "QLabel#panelSummary { background: #365642; color: #fff5d8; padding: 4px 8px; }"
            "QLabel#tableHeading { color: #63533b; border-bottom: 2px solid #ab926d; }"
            "QListWidget { background: #fff4d8; color: #382f29; border: 2px solid #99754e; }"
            "QListWidget::item { padding: 2px 5px; }"
            "QListWidget::item:selected, QListWidget::item:selected:active { background: #dde6bf; color: #24392a; }"
            "QTabWidget::pane { border: 0; }"
            "QTabBar::tab { background: #dfc99a; color: #4b3b2e; border: 1px solid #99754e; padding: 4px 12px; }"
            "QTabBar::tab:selected { background: #446747; color: #fff7dc; }"
            "QPushButton { color: #3e3328; background: #f8e9c1; border: 2px solid #9a7850;"
            " border-bottom: 3px solid #795b40; padding: 4px 9px; }"
            "QPushButton:hover { background: #ffefb9; }"
            "QPushButton:focus { border-color: #355e43; }"
            "QPushButton:pressed { background: #dbc393; }"
            "QPushButton:disabled { color: #817b6c; border-color: #b7ab98; background: #e0d6bd; }"));
    }

    static QPixmap slotIcon(const QPixmap &item) {
        static const QPixmap slot = QPixmap(QStringLiteral(":/farm-ui/slots.png")).copy(8, 8, 32, 34);
        QPixmap pixmap = slot;
        QPainter painter(&pixmap);
        painter.drawPixmap(QRect(4, 4, 24, 24), item);
        return pixmap;
    }

    void tableHeading(QGridLayout *grid, int column, const QString &text, int span = 1) {
        auto *label = new QLabel(text, this);
        label->setObjectName(QStringLiteral("tableHeading"));
        grid->addWidget(label, 0, column, 1, span);
    }

    void productCell(QGridLayout *grid, int row, const QString &name, const QPixmap &icon,
                     const QString &tooltip = {}) {
        auto *image = new QLabel(this);
        image->setProperty("tradeRow", row);
        image->setPixmap(slotIcon(icon));
        image->setToolTip(tooltip);
        grid->addWidget(image, row, 0);
        auto *label = new QLabel(name, this);
        label->setToolTip(tooltip);
        grid->addWidget(label, row, 1);
    }

    QPushButton *makeButton(const QString &text, const QString &objectName) {
        auto *button = new QPushButton(text, this);
        button->setObjectName(objectName);
        button->setAutoDefault(false);
        button->setFocusPolicy(Qt::StrongFocus);
        return button;
    }

    void addFooter(const QString &message, QPushButton *action = nullptr) {
        m_message = new QLabel(message, this);
        m_message->setObjectName(QStringLiteral("panelMessage"));
        m_message->setWordWrap(true);
        body->addWidget(m_message);
        auto *buttons = new QHBoxLayout;
        buttons->addStretch();
        if (action)
            buttons->addWidget(action);
        auto *closeButton = makeButton(QStringLiteral("关闭"), QStringLiteral("closePanel"));
        connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
        buttons->addWidget(closeButton);
        body->addLayout(buttons);

        // A held Enter/Space must never buy or sell repeatedly. Observe child
        // key events as well because the focused button normally receives them.
        for (QWidget *child : findChildren<QWidget *>())
            child->installEventFilter(this);
        installEventFilter(this);
    }

    bool eventFilter(QObject *object, QEvent *event) override {
        if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
            const auto *key = static_cast<QKeyEvent *>(event);
            if (key->isAutoRepeat() && (key->key() == Qt::Key_Return ||
                key->key() == Qt::Key_Enter || key->key() == Qt::Key_Space ||
                key->key() == Qt::Key_Escape))
                return true;
            if (event->type() == QEvent::KeyPress &&
                (key->key() == Qt::Key_Enter || key->key() == Qt::Key_Return)) {
                if (auto *button = qobject_cast<QPushButton *>(object)) {
                    button->click();
                    return true;
                }
            }
        }
        return QDialog::eventFilter(object, event);
    }

    QVBoxLayout *body;
    QLabel *summary;

private:
    bool m_tradeStyle = false;
    QLabel *m_message = nullptr;
};

#endif
