#ifndef FARMICONS_H
#define FARMICONS_H

#include "farm/cropdefinition.h"
#include <QPixmap>
#include <QPainter>
#include "economy/economyservice.h"
#include <QStringList>

namespace FarmIcons {
inline QRectF hotbarCell(const QRectF &frame, int index) {
    return QRectF(frame.left()+(10+index*48)*frame.width()/352.0,
                  frame.top()+30*frame.height()/96.0,
                  42*frame.width()/352.0,43*frame.height()/96.0);
}

inline void centeredIcon(QPainter &p, const QPixmap &icon, const QRectF &cell, int size) {
    if (icon.isNull()) return;
    const QImage image = icon.toImage();
    int left=image.width(), top=image.height(), right=-1, bottom=-1;
    for (int y=0; y<image.height(); ++y) for (int x=0; x<image.width(); ++x) {
        if (qAlpha(image.pixel(x,y))) {
            left=qMin(left,x); top=qMin(top,y); right=qMax(right,x); bottom=qMax(bottom,y);
        }
    }
    if (right<left) return;
    const QRect source(left,top,right-left+1,bottom-top+1);
    const qreal scale = qreal(size)/qMax(icon.width(),icon.height());
    const QSize target(qMax(1,qRound(source.width()*scale)),qMax(1,qRound(source.height()*scale)));
    const QRect destination(QPoint(qRound(cell.center().x()-target.width()/2.0),
                                  qRound(cell.center().y()-target.height()/2.0)),target);
    p.setRenderHint(QPainter::SmoothPixmapTransform,false);
    p.drawPixmap(destination,icon,source);
}

inline QPixmap hotbarBackground(int selected) {
    static const QVector<QPixmap> backgrounds = [] {
        const QPixmap original(QStringLiteral(":/farm-ui/hotbar.png"));
        const QPixmap slotSheet(QStringLiteral(":/farm-ui/slots.png"));
        QVector<QPixmap> result{original};
        const QColor backing = original.toImage().pixelColor(56,60);
        for (int i=0; i<7; ++i) {
            QPixmap frame = original;
            QPainter p(&frame);
            const QRect cell(10+i*48,30,42,43);
            // Remove the entire light slot, including its border and corners,
            // before drawing its replacement. Transparent corners see wood.
            p.fillRect(cell,backing);
            p.drawPixmap(cell,slotSheet,QRect(99,98,42,44));
            p.end();
            result.append(frame);
        }
        return result;
    }();
    return backgrounds.at(selected >= 0 && selected < 7 ? selected+1 : 0);
}

inline void selectedSlot(QPainter &painter, const QRect &target) {
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.drawPixmap(target, QPixmap(QStringLiteral(":/farm-ui/slots.png")), QRect(99,98,42,44));
}

inline QPixmap crop(const CropIcon &icon) {
    return QPixmap(icon.resourcePath).copy(icon.sourceRect);
}

inline QPixmap fertilizer() {
    return QPixmap(QStringLiteral(":/farm-ui/soil-items.png")).copy(0, 0, 16, 16);
}

inline QPixmap stackIcon(int item) {
    if(item==StackItem::Wood) return QPixmap(QStringLiteral(":/farm-ui/wood-tools.png")).copy(0,32,16,16);
    if (item==StackItem::Axe) return QPixmap(QStringLiteral(":/tilesets/sprout/Basic_tools_and_meterials.png")).copy(16,0,16,16);
    if (item==StackItem::Hoe || item==StackItem::Water)
        return QPixmap(QStringLiteral(":/tilesets/sprout/Basic_tools_and_meterials.png"))
            .copy(item==StackItem::Hoe ? 32 : 0,0,16,16);
    if (item==StackItem::Fertilizer) return fertilizer();
    if (item/1000==2) return crop(CropCatalog::definition(StackItem::crop(item)).seedIcon);
    if (item/1000==3) return crop(CropCatalog::definition(StackItem::crop(item)).harvestIcon);
    return {};
}

inline QString stackName(const InventoryStack &stack) {
    if (!stack.item) return {};
    if(stack.item==StackItem::Wood) return QStringLiteral("木材 × %1").arg(stack.quantity);
    QString name;
    if (StackItem::isTool(stack.item) || stack.item==StackItem::Fertilizer) name=toolName(StackItem::tool(stack.item));
    else {
        name=CropCatalog::definition(StackItem::crop(stack.item)).localizedName;
        if (stack.item/1000==2) name+=QStringLiteral("种子");
    }
    return name+QStringLiteral(" × %1").arg(stack.quantity);
}

inline QString itemDescription(int item) {
    if (!item || !StackItem::valid(item)) return {};
    if (item == StackItem::Axe)
        return QStringLiteral("斧头\n靠近树干，挥斧 4 次砍倒树，再挥 2 次清除树桩，两次都能得到木材。"
                              "\n商店强化后，砍树只需 2 斧，清除树桩只需 1 斧。");
    if(item==StackItem::Wood) return QStringLiteral("木材\n砍树所得的材料，可在背包中移动、拆分或存入储物箱。");
    if (item == StackItem::Hoe) return QStringLiteral("锄头\n将可耕草地翻成耕地，再种下种子。");
    if (item == StackItem::Water)
        return QStringLiteral("水壶\n装满可浇 %1 格地。在水井旁按 F 补满。\n商店扩容后可浇 %2 格，升级时会装满水。")
            .arg(EconomyState::waterCapacity(false)).arg(EconomyState::waterCapacity(true));
    if (item == StackItem::Fertilizer)
        return QStringLiteral("基础肥料\n用于未成熟作物，每株限用一次。\n下次累计满 1 个生长日时额外成长 1 天。");
    const auto &crop = CropCatalog::definition(StackItem::crop(item));
    if (item / 1000 == 2) {
        QStringList seasons;
        for (Season season : {Season::Spring, Season::Summer, Season::Autumn, Season::Winter})
            if (CropCatalog::canPlant(crop.type, season)) seasons.append(SeasonCalendar::seasonName(season));
        QString text = QStringLiteral("%1种子\n生长周期：%2 个生长日\n可播种：%3\n适季每浇水 1 天计 1 个生长日，缺水时暂停。")
            .arg(crop.localizedName).arg(crop.growthDays).arg(seasons.join(QStringLiteral("、")));
        if (crop.plantingSeasons != SeasonCalendar::AllSeasons)
            text += QStringLiteral("\n换季后植株保留，非适季每浇水 2 天计 1 个生长日。");
        return text + QStringLiteral("\n每次收获：%1 份%2\n种子单价：%3 金币")
            .arg(crop.harvestYield).arg(crop.localizedName).arg(crop.seedPrice);
    }
    return QStringLiteral("%1\n农产品，可携带到收购点出售，也可存入箱子。\n出售单价：%2 金币")
        .arg(crop.localizedName).arg(crop.sellPrice);
}

inline QString itemTooltip(int item) {
    const auto lines = itemDescription(item).split(QLatin1Char('\n'));
    if (lines.isEmpty() || lines.first().isEmpty()) return {};
    QString html = QStringLiteral("<div style='max-width:300px;color:#563b2b'><b>%1</b>")
        .arg(lines.first().toHtmlEscaped());
    for (int i=1; i<lines.size(); ++i) html += QStringLiteral("<br>%1").arg(lines[i].toHtmlEscaped());
    return html + QStringLiteral("</div>");
}
} // namespace FarmIcons

#endif
