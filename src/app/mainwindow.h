#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "economy/economyservice.h"
#include "core/gameclock.h"
#include "ui/itembar.h"
#include "map/mapwidget.h"
#include "player/rpg.h"
#include "storage/savemanager.h"
#include "map/seasonappearance.h"

#include <QCursor>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QMainWindow>
#include <QPointer>

class DayNightOverlay;
class FarmPanel;
class QDialog;
class FarmMenus;
class QStackedWidget;
class QMediaPlayer;
class QMessageBox;
class QTimer;
class SceneTransition;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    int currentGameHour() const { return int(m_committedMinutes / 60 % 24); }
    int currentGameDay() const { return int(m_committedMinutes / (24 * 60)); }
    int currentGameMinute() const { return int(m_committedMinutes % 60); }

signals:
    void wakeSaved();

private slots:
    void updateTime();
    void sleepUntilMorning();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    enum class ClockEvent { Tick, WakeUp, Load, Save };

    enum class Page { Home, Farm, Pause };
    void enterFarm();
    void pauseFarm();
    void resumeFarm();
    void showHome();
    bool readFarmSave(const QString &source, GameSaveData *data, QString *loadedPath) const;
    bool applyClock(ClockEvent event);
    DayAdvanceResult processDayTransitions();
    void presentTime();
    void updateGameScale();
    void updateHud();
    void showStatus(const QString &message);
    void openInventory();
    void openSeedShop();
    void openMarket();
    void openStorage();
    void showFarmPanel(QDialog *dialog);
    void refreshOpenPanel();
    void refreshGameplayInput();
    void finishTrade(FarmPanel *dialog, const QString &receipt);
    bool saveGame(bool showError = true);
    void loadGame();
    bool restoreGame(const GameSaveData &data);
    bool isCompatibleSave(const GameSaveData &data) const;

    QStackedWidget *m_pages;
    FarmMenus *m_menus;
    QTimer *m_tick;
    QTimer *m_autosave;
    QMediaPlayer *m_music;
    Page m_page = Page::Home;
    GameClock m_gameClock;
    // Map actions and saves use the last sampled time whose days have been settled.
    qint64 m_committedMinutes = GameConstants::WakeUpHour * 60;
    int m_lastProcessedDay = 0;
    int m_visibleDay = -1;
    bool m_saveEnabled = true;
    EconomyService m_economy;
    QPointer<QDialog> m_panelOpen;
    QPointer<QMessageBox> m_closePrompt;
    bool m_discardOnClose = false;
    QPointer<QWidget> m_panelFocusBeforeFade;
    bool m_transitionActive = false;
    const QRect m_marketArea{28, 18, 2, 2};
    const QRect m_storageArea{31, 21, 1, 1};
    const QRect m_shopArea{33, 18, 2, 2};
    const QRect m_bedArea{25, 8, 2, 2};
    QWidget *m_gameCanvas;
    QGraphicsScene *m_gameScene;
    QGraphicsView *m_gameView;
    MapWidget *mapWidget;
    ImageMovementWidget *character;
    ItemBar *itemBar;
    SeasonAppearance m_seasons;
    DayNightOverlay *m_daylight;
    SceneTransition *m_transition;
    QLabel *m_coinsLabel;
    QLabel *m_inventoryLabel;
    QLabel *m_suppliesLabel;
    QLabel *m_statusLabel;
    QLabel *timeLabel;
    QLabel *dayLabel;
    QCursor customCursor;
    QCursor clickCursor;
};

#endif // MAINWINDOW_H
