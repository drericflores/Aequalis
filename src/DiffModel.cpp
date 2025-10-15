
#include "Aequalis/DiffModel.hpp"
#include <QDateTime>

namespace aequalis {

static QString actionToString(Action a) {
  switch (a) {
    case Action::Identical: return "identical";
    case Action::CopyNew: return "copy_new";
    case Action::CopyNewer: return "copy_newer";
    case Action::CopyMismatch: return "copy_mismatch";
    case Action::SkipDestNewer: return "skip_dest_newer";
    case Action::OnlyInDest: return "only_in_dest";
    case Action::TypeMismatch: return "type_mismatch";
  }
  return "";
}

DiffModel::DiffModel(QObject* parent): QAbstractTableModel(parent) {}

int DiffModel::rowCount(const QModelIndex&) const { return static_cast<int>(m_diffs.size()); }
int DiffModel::columnCount(const QModelIndex&) const { return 7; }

QVariant DiffModel::data(const QModelIndex& idx, int role) const {
  if (!idx.isValid() || idx.row() >= rowCount()) return {};
  const auto& d = m_diffs[static_cast<size_t>(idx.row())];
  if (role == Qt::DisplayRole) {
    switch (idx.column()) {
      case 0: return d.relpath;
      case 1: return actionToString(d.action);
      case 2: return d.reason;
      case 3: return d.src.exists ? QDateTime::fromSecsSinceEpoch(static_cast<qint64>(d.src.mtime)).toString("yyyy-MM-dd HH:mm:ss") : "—";
      case 4: return d.dst.exists ? QDateTime::fromSecsSinceEpoch(static_cast<qint64>(d.dst.mtime)).toString("yyyy-MM-dd HH:mm:ss") : "—";
      case 5: return d.src.exists ? QVariant::fromValue<qlonglong>(static_cast<qlonglong>(d.src.size)) : QVariant("—");
      case 6: return d.dst.exists ? QVariant::fromValue<qlonglong>(static_cast<qlonglong>(d.dst.size)) : QVariant("—");
    }
  }
  if (role == Qt::TextAlignmentRole && (idx.column()==5 || idx.column()==6)) return QVariant(Qt::AlignRight | Qt::AlignVCenter);
  return {};
}

QVariant DiffModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (role != Qt::DisplayRole) return {};
  if (orientation == Qt::Horizontal) {
    switch (section) {
      case 0: return "Relpath"; case 1: return "Action"; case 2: return "Reason";
      case 3: return "Src mtime"; case 4: return "Dst mtime"; case 5: return "Src size"; case 6: return "Dst size";
    }
  }
  return section + 1;
}

void DiffModel::setDiffs(std::vector<DiffItem> diffs) {
  beginResetModel();
  m_diffs = std::move(diffs);
  endResetModel();
}

} // namespace aequalis
