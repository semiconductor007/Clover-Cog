#ifndef SCENETRANSITION_H
#define SCENETRANSITION_H

#include <QColor>
#include <QPixmap>
#include <QSequentialAnimationGroup>
#include <QStringList>
#include <QWidget>

class QPushButton;

class CanvasOverlay : public QWidget {
protected:
    explicit CanvasOverlay(QWidget *canvas);
    bool eventFilter(QObject *watched, QEvent *event) override;
};

// Stack above the scene and player and below the toolbar and HUD.
class DayNightOverlay : public CanvasOverlay {
public:
    explicit DayNightOverlay(QWidget *canvas);
    // Total elapsed game minutes, including the date used by SeasonCalendar.
    void setGameMinutes(qint64 minutes);
    static QColor tintForMinutes(qint64 minutes);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor m_tint = Qt::transparent;
};

class SceneTransition : public CanvasOverlay {
    Q_OBJECT
public:
    explicit SceneTransition(QWidget *canvas);
    // Capture the visible scene before the caller commits a time change.
    bool start();
    bool isActive() const;

signals:
    void blackReached();
    void finished();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QSequentialAnimationGroup m_animation;
    QPixmap m_outgoingFrame;
    int m_blackAlpha = 0;
};

class SleepConfirmationOverlay : public CanvasOverlay {
    Q_OBJECT
public:
    explicit SleepConfirmationOverlay(QWidget *viewport);
    void present(const QString &wakeDate);

signals:
    void restRequested();
    void cancelRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QPushButton *m_rest;
    QPushButton *m_cancel;
    QString m_wakeDate;
};

class WakeSummaryPage : public QWidget {
    Q_OBJECT
public:
    explicit WakeSummaryPage(QWidget *parent);
    void present(const QString &title, const QString &date, const QString &dateChange,
                 const QStringList &rows, const QString &saveStatus, bool saved, bool canContinue);
    void focusContinue();

signals:
    void continueRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QPushButton *m_continue;
    QString m_title;
    QString m_date;
    QString m_dateChange;
    QStringList m_rows;
    QString m_saveStatus;
    bool m_saved = false;
};

#endif // SCENETRANSITION_H
