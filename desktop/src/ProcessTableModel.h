#pragma once
#include <QAbstractTableModel>
#include <QVector>
#include "agentclient.h"

class ProcessTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column { Name, Pid, Cpu, Memory, ColumnCount };

    using QAbstractTableModel::QAbstractTableModel;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void updateProcesses(QVector<ProcessInfo> incoming);

private:
    QVector<ProcessInfo> m_rows; // ținute sortate după PID
};