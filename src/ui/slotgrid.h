#ifndef SLOTGRID_H
#define SLOTGRID_H

#include "farmicons.h"
#include <QApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QToolTip>
#include <QHelpEvent>
#include <QLabel>
#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <functional>

// Read-only view and drop routing. Only the controller commits a StackMove.
struct SlotDropSurface {
    QWidget *widget;
    bool chest;
    std::function<int(QPoint)> indexAt;
    std::function<void(StackMove)> drop;
    static QVector<SlotDropSurface *> &all() { static QVector<SlotDropSurface *> list; return list; }
    SlotDropSurface(QWidget *w, bool c, std::function<int(QPoint)> at) : widget(w),chest(c),indexAt(std::move(at)) { all().append(this); }
    ~SlotDropSurface() { all().removeOne(this); }
    SlotDropSurface(const SlotDropSurface &)=delete;
    static QWidget *root(QWidget *w) {
        while(w->parentWidget()) w=w->parentWidget();
        if (auto *proxy=w->graphicsProxyWidget())
            if(proxy->scene() && !proxy->scene()->views().isEmpty()) return proxy->scene()->views().first()->window();
        return w;
    }
    static QPoint globalPoint(QWidget *widget,QPoint point) {
        QWidget *canvas=widget;while(canvas->parentWidget()) canvas=canvas->parentWidget();
        if(auto *proxy=canvas->graphicsProxyWidget()) if(proxy->scene() && !proxy->scene()->views().isEmpty()) {
            auto *view=proxy->scene()->views().first();
            return view->viewport()->mapToGlobal(view->mapFromScene(proxy->mapToScene(widget->mapTo(canvas,point))));
        }
        return widget->mapToGlobal(point);
    }
    static QPoint localPoint(QWidget *widget,QPoint global) {
        QWidget *canvas=widget;while(canvas->parentWidget()) canvas=canvas->parentWidget();
        if(auto *proxy=canvas->graphicsProxyWidget()) if(proxy->scene() && !proxy->scene()->views().isEmpty()) {
            auto *view=proxy->scene()->views().first();
            return widget->mapFrom(canvas,proxy->mapFromScene(view->mapToScene(view->viewport()->mapFromGlobal(global))).toPoint());
        }
        return widget->mapFromGlobal(global);
    }
    static void deliver(QWidget *source, QPoint global, StackMove command) {
        for (auto *target : all()) {
            if (!target->widget->isVisible() || root(source)!=root(target->widget)) continue;
            if (target->widget->window()!=source->window() &&
                QRect(source->window()->mapToGlobal(QPoint()),source->window()->size()).contains(global)) continue;
            const QPoint local=localPoint(target->widget,global);
            if (!target->widget->visibleRegion().contains(local)) continue;
            const int index=target->indexAt(local);
            if (index<0 || !target->drop) continue;
            command.targetChest=target->chest; command.target=index;
            target->drop(command); return;
        }
    }
};

class SlotGrid : public QWidget {
public:
    std::function<void(StackMove)> moveRequested;
    std::function<void(int)> slotChosen;
    explicit SlotGrid(bool chest=false, QWidget *parent=nullptr)
        : QWidget(parent),m_chest(chest),m_surface(this,chest,[this](QPoint p){return indexAt(p);}) {
        setFocusPolicy(Qt::StrongFocus); setMouseTracking(true);
        m_surface.drop=[this](StackMove command) { if (moveRequested) moveRequested(command); };
        m_ghost=new QLabel(this,Qt::ToolTip);
        m_ghost->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_ghost->setAttribute(Qt::WA_ShowWithoutActivating);
    }
    ~SlotGrid() override = default;
    void setState(QVector<InventoryStack> cells, quint64 revision, int active=-1) {
        m_cells=std::move(cells); m_revision=revision; m_active=active;
        const int rows=(m_cells.size()+6)/7;
        setFixedSize(522,m_chest ? rows*72+12 : 144+qMax(0,rows-1)*72);
        update();
    }
    const QVector<InventoryStack> &cells() const { return m_cells; }
    QRect cellRect(int index) const {
        if (index<0 || index>=m_cells.size()) return {};
        if (!m_chest && index<7) return FarmIcons::hotbarCell(QRectF(0,0,522,144),index).toAlignedRect();
        return QRect(12+(index%7)*72,(m_chest ? 6+(index/7)*72 : 144+(index/7-1)*72),66,66);
    }
    int indexAt(QPoint point) const {
        for (int i=0; i<m_cells.size(); ++i) if (cellRect(i).contains(point)) return i;
        return -1;
    }
    void cancelDrag() { m_pressed=false; m_dragging=false; m_ghost->hide(); unsetCursor(); update(); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::SmoothPixmapTransform,false);
        if (!m_chest) p.drawPixmap(QRect(0,0,522,144),FarmIcons::hotbarBackground(m_active));
        const QPixmap frames(QStringLiteral(":/farm-ui/slots.png"));
        for (int i=0; i<m_cells.size(); ++i) {
            const QRect cell=cellRect(i);
            if (m_chest || i>=7) {
                if (i==m_cursor && hasFocus()) FarmIcons::selectedSlot(p,cell);
                else p.drawPixmap(cell,frames,QRect(3,98,42,44));
            }
            const auto &stack=m_cells[i];
            if (!stack.item) continue;
            FarmIcons::centeredIcon(p,FarmIcons::stackIcon(stack.item),cell,36);
            if (!StackItem::isTool(stack.item)) {
                QFont f=font(); f.setBold(true); f.setPixelSize(13); p.setFont(f); p.setPen(QColor("#513521"));
                p.drawText(cell.adjusted(2,0,-4,-3),Qt::AlignRight|Qt::AlignBottom,QString::number(stack.quantity));
            }
        }
    }
    bool event(QEvent *event) override {
        if (event->type()==QEvent::ToolTip) {
            auto *help=static_cast<QHelpEvent *>(event); const int i=indexAt(help->pos());
            if (i>=0 && m_cells[i].item) QToolTip::showText(help->globalPos(),FarmIcons::stackName(m_cells[i]),this);
            else QToolTip::hideText();
            return true;
        }
        if (event->type()==QEvent::Hide || (event->type()==QEvent::EnabledChange && !isEnabled())) cancelDrag();
        return QWidget::event(event);
    }
    void mousePressEvent(QMouseEvent *event) override {
        event->accept(); if (!isEnabled()) return;
        m_cursor=indexAt(event->position().toPoint()); setFocus(); update();
        if (m_cursor<0 || !m_cells[m_cursor].item || (event->button()!=Qt::LeftButton && event->button()!=Qt::RightButton)) return;
        m_press=event->position().toPoint(); m_pressed=true; m_dragging=false; m_right=event->button()==Qt::RightButton;
        m_command={}; m_command.sourceChest=m_chest; m_command.source=m_cursor; m_command.revision=m_revision;
        m_command.expected=m_cells[m_cursor];
        m_command.quantity=m_right ? ((event->modifiers() & Qt::ShiftModifier) ?
            m_command.expected.quantity/2+m_command.expected.quantity%2 : 1) : m_command.expected.quantity;
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        if (!m_pressed) return;
        if (!m_dragging && (event->position().toPoint()-m_press).manhattanLength()>=QApplication::startDragDistance()) {
            m_dragging=true;
            QPixmap ghost(48,52); ghost.fill(Qt::transparent); QPainter p(&ghost);
            FarmIcons::centeredIcon(p,FarmIcons::stackIcon(m_command.expected.item),QRect(0,0,48,40),32);
            p.setPen(Qt::white); p.drawText(QRect(0,32,48,20),Qt::AlignCenter,QString::number(m_command.quantity)); p.end();
            m_ghost->setPixmap(ghost); m_ghost->resize(ghost.size());
        }
        if (m_dragging) { m_ghost->move(event->globalPosition().toPoint()+QPoint(10,10)); m_ghost->show(); }
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        event->accept(); if (!m_pressed) return;
        const bool dragging=m_dragging; const StackMove command=m_command; const bool right=m_right;
        cancelDrag();
        if (dragging) { SlotDropSurface::deliver(this,event->globalPosition().toPoint(),command); return; }
        if (indexAt(event->position().toPoint())!=command.source) return;
        if (right && !StackItem::isTool(command.expected.item)) {
            auto split=command; split.targetChest=m_chest;
            for (int i=0; i<m_cells.size(); ++i) if (!m_cells[i].item) {
                split.target=i; if (moveRequested) moveRequested(split); break;
            }
        } else if (!right && slotChosen) slotChosen(command.source);
    }
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key()==Qt::Key_Escape && m_pressed) { cancelDrag(); event->accept(); return; }
        int next=m_cursor<0 ? 0 : m_cursor;
        if (event->key()==Qt::Key_Left) --next;
        else if (event->key()==Qt::Key_Right) ++next;
        else if (event->key()==Qt::Key_Up) next-=7;
        else if (event->key()==Qt::Key_Down) next+=7;
        else if (event->key()==Qt::Key_Return || event->key()==Qt::Key_Enter) {
            if (!event->isAutoRepeat() && slotChosen && m_cursor>=0) slotChosen(m_cursor);
            event->accept(); return;
        } else { QWidget::keyPressEvent(event); return; }
        m_cursor=qBound(0,next,qMax(0,int(m_cells.size())-1)); update(); event->accept();
    }
private:
    bool m_chest, m_pressed=false,m_dragging=false,m_right=false;
    QVector<InventoryStack> m_cells;
    quint64 m_revision=0;
    int m_active=-1,m_cursor=-1;
    QPoint m_press;
    StackMove m_command;
    SlotDropSurface m_surface;
    QLabel *m_ghost;
};

#endif
