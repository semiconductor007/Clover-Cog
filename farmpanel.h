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

    void productCell(QGridLayout *grid, int row, const QString &name, const QPixmap &icon) {
        auto *image = new QLabel(this);
        image->setPixmap(slotIcon(icon));
        grid->addWidget(image, row, 0);
        grid->addWidget(new QLabel(name, this), row, 1);
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
    QLabel *m_message = nullptr;
};

#endif
