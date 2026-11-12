#ifndef FARMMENUS_H
#define FARMMENUS_H

#include <QStackedWidget>

class QLabel;
class QPushButton;

class FarmMenus : public QStackedWidget {
    Q_OBJECT
public:
    explicit FarmMenus(const QPixmap &farmPicture, QWidget *parent = nullptr);
    void showHome(bool canContinue, const QString &notice);
    void showPause(const QString &notice = {});

signals:
    void enterRequested();
    void resumeRequested();
    void saveAndLeaveRequested();
    void exitRequested();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void showGuide();
    void leaveGuide();

    QWidget *m_home;
    QWidget *m_pause;
    QWidget *m_guide;
    QWidget *m_guideOrigin = nullptr;
    QLabel *m_homeNotice;
    QLabel *m_pauseNotice;
    QPushButton *m_enter;
    QPushButton *m_resume;
    QPushButton *m_homeGuide;
    QPushButton *m_pauseGuide;
    QPushButton *m_back;
};

#endif
