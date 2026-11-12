#ifndef STORAGEDIALOG_H
#define STORAGEDIALOG_H

#include "economy/economyservice.h"
#include "farmpanel.h"

class QTableWidget;

class StorageDialog : public FarmPanel {
    Q_OBJECT
public:
    explicit StorageDialog(QWidget *parent = nullptr);
    void setState(const EconomyService &economy);

signals:
    void transferRequested(StorageItemKind kind, CropType crop, StorageDirection direction, int quantity);

private:
    struct Row {
        StorageItemKind kind;
        CropType crop;
    };
    void addRow(StorageItemKind kind, CropType crop, const QString &name, const QPixmap &icon);
    void updateButtons();
    void requestTransfer(StorageDirection direction, bool all);

    QTableWidget *m_table;
    QVector<Row> m_rows;
    QPushButton *m_depositOne;
    QPushButton *m_depositAll;
    QPushButton *m_withdrawOne;
    QPushButton *m_withdrawAll;
};

#endif
