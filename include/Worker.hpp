
#ifndef AEQUALIS_WORKER_HPP
#define AEQUALIS_WORKER_HPP
#include "Aequalis/Types.hpp"
#include <QThread>
#include <QSet>
#include <atomic>

namespace aequalis {

class CompareWorker : public QThread {
  Q_OBJECT
public:
  CompareWorker(bool filesMode,
                QString src,
                QString dst,
                QSet<QString> ignores,
                QObject* parent=nullptr);
  void cancel();

signals:
  void done(std::vector<DiffItem> diffs);
  void failed(QString error);
  void phase(QString label);
  void progressRange(int min, int max);
  void progressValue(int value);

protected:
  void run() override;

private:
  bool m_filesMode{false};
  QString m_src, m_dst;
  QSet<QString> m_ignores;
  std::atomic_bool m_cancel{false};
};

} // namespace aequalis

#endif // AEQUALIS_WORKER_HPP
