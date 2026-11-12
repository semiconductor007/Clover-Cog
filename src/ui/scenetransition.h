#ifndef SCENETRANSITION_H
#define SCENETRANSITION_H

#include <QColor>
#include <QPixmap>
#include <QSequentialAnimationGroup>
#include <QWidget>

class CanvasOverlay : public QWidget {
protected:
    explicit CanvasOverlay(QWidget *canvas);
    bool eventFilter(QObject *watched, QEvent *event) override;
};

// Stack above the scene and player and below the toolbar and HUD.
class DayNightOverlay : public CanvasOverlay {
public:
    explicit DayNightOverlay(QWidget *canvas);
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

#endif // SCENETRANSITION_H
