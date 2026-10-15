#include "processtablemodel.h"
#include <algorithm>

int ProcessTableModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

int ProcessTableModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant ProcessTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};

    const ProcessInfo& p = m_rows[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Name:   return p.name;
        case Pid:    return p.pid;
        case Cpu:    return QString::number(p.cpu, 'f', 1) + " %";
        case Memory: return QString::number(p.memoryMB, 'f', 1) + " MB";
        }
    }

    if (role == Qt::UserRole) {
        switch (index.column()) {
        case Name:   return p.name;
        case Pid:    return p.pid;
        case Cpu:    return p.cpu;
        case Memory: return p.memoryMB;
        }
    }

    if (role == Qt::TextAlignmentRole && index.column() != Name)
        return int(Qt::AlignRight | Qt::AlignVCenter);

    return {};
}

QVariant ProcessTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section) {
    case Name:   return "Proces";
    case Pid:    return "PID";
    case Cpu:    return "CPU";
    case Memory: return "Memorie";
    }
    return {};
}

void ProcessTableModel::updateProcesses(QVector<ProcessInfo> incoming)
{
    std::sort(incoming.begin(), incoming.end(),
        [](const ProcessInfo& a, const ProcessInfo& b) { return a.pid < b.pid; });

    int i = 0, j = 0;
    while (i < m_rows.size() || j < incoming.size()) {
        const bool oldOnly = j >= incoming.size()
            || (i < m_rows.size() && m_rows[i].pid < incoming[j].pid);
        const bool newOnly = !oldOnly
            && (i >= m_rows.size() || incoming[j].pid < m_rows[i].pid);

        if (oldOnly) {
            beginRemoveRows({}, i, i);
            m_rows.removeAt(i);
            endRemoveRows();
        }
        else if (newOnly) {
            beginInsertRows({}, i, i);
            m_rows.insert(i, incoming[j]);
            endInsertRows();
            ++i; ++j;
        }
        else {
            m_rows[i] = incoming[j];
            ++i; ++j;
        }
    }

    if (!m_rows.isEmpty())
        emit dataChanged(index(0, 0), index(rowCount() - 1, ColumnCount - 1));
}