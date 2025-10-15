
#include "Aequalis/Worker.hpp"
#include "Aequalis/Scanner.hpp"
#include <QtConcurrent>

namespace aequalis {

CompareWorker::CompareWorker(bool filesMode, QString src, QString dst, QSet<QString> ignores, QObject* parent)
  : QThread(parent), m_filesMode(filesMode), m_src(std::move(src)), m_dst(std::move(dst)), m_ignores(std::move(ignores)) {}

void CompareWorker::cancel() { m_cancel = true; }

void CompareWorker::run() {
  try {
    if (m_filesMode) {
      emit phase("Comparing file…");
      emit progressRange(0, 1);
      auto di = compareFiles(m_src, m_dst);
      emit progressValue(1);
      emit done(std::vector<DiffItem>{std::move(di)});
      return;
    }

    emit phase("Scanning…");
    emit progressRange(0, 0); // indeterminate

    // Scan source and destination concurrently
    auto srcFuture = QtConcurrent::run([&]{ return fastListFiles(m_src, m_ignores, [this]{ return m_cancel.load(); }); });
    auto dstFuture = QtConcurrent::run([&]{ return fastListFiles(m_dst, m_ignores, [this]{ return m_cancel.load(); }); });
    MetaMap sm = srcFuture.result();
    MetaMap dm = dstFuture.result();

    // Merge keys
    QSet<QString> keys;
    for (auto it = sm.constBegin(); it != sm.constEnd(); ++it) keys.insert(it.key());
    for (auto it = dm.constBegin(); it != dm.constEnd(); ++it) keys.insert(it.key());
    const int total = keys.size();

    emit phase("Comparing…");
    emit progressRange(0, total);

    std::vector<DiffItem> diffs; diffs.reserve(total);
    int i = 0;
    for (const auto& r : keys) {
      if (m_cancel.load()) break;
      const QString sp = m_src + "/" + r;
      const QString dp = m_dst + "/" + r;
      if (sm.contains(r) && !dm.contains(r)) {
        diffs.push_back(DiffItem{r, Action::CopyNew, "Missing in dest", sm.value(r), FileMeta{}});
      } else if (!sm.contains(r) && dm.contains(r)) {
        diffs.push_back(DiffItem{r, Action::OnlyInDest, "Only in dest", FileMeta{}, dm.value(r)});
      } else {
        auto di = compareFiles(sp, dp);
        di.relpath = r;
        diffs.push_back(std::move(di));
      }
      ++i; emit progressValue(i);
    }

    emit done(std::move(diffs));
  } catch (const std::exception& e) {
    emit failed(QString::fromUtf8(e.what()));
  }
}

} // namespace aequalis
