
#ifndef AEQUALIS_DIFFMODEL_HPP
#define AEQUALIS_DIFFMODEL_HPP
#include "Aequalis/Types.hpp"
#include <QAbstractTableModel>
#include <vector>

namespace aequalis {

class DiffModel : public QAbstractTableModel {
  Q_OBJECT
public:
  explicit DiffModel(QObject* parent=nullptr);
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

  void setDiffs(std::vector<DiffItem> diffs);
  const std::vector<DiffItem>& diffs() const { return m_diffs; }

private:
  std::vector<DiffItem> m_diffs;
};

} // namespace aequalis

#endif // AEQUALIS_DIFFMODEL_HPP
