
#include "Aequalis/Scanner.hpp"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QDateTime>
#include <QtConcurrent>
#include <filesystem>
#include <system_error>
#include <chrono>
#include <cmath>

namespace fs = std::filesystem;

namespace aequalis {

static double toSeconds(const fs::file_time_type& tp) {
#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L
  auto s = std::chrono::clock_cast<std::chrono::system_clock>(tp).time_since_epoch();
#else
  auto s = std::chrono::time_point_cast<std::chrono::seconds>(tp).time_since_epoch();
#endif
  return std::chrono::duration<double>(s).count();
}

static FileMeta metaFromPath(const fs::path& p) {
  FileMeta fm{};
  std::error_code ec;
  if (!fs::exists(p, ec)) return fm;
  fm.exists = true;
  fm.isFile = fs::is_regular_file(p, ec);
  if (fm.isFile) {
    fm.size = fs::file_size(p, ec);
  }
  auto ftime = fs::last_write_time(p, ec);
  if (!ec) fm.mtime = toSeconds(ftime);
  return fm;
}

MetaMap fastListFiles(const QString& root, const QSet<QString>& ignoreNames, const CancelFn& cancel) {
  MetaMap out;
  fs::path rootp = fs::u8path(root.toStdString());
  std::error_code ec;
  if (!fs::exists(rootp, ec) || !fs::is_directory(rootp, ec)) return out;

  std::vector<fs::path> stack{rootp};
  while (!stack.empty()) {
    if (cancel && cancel()) break;
    fs::path d = stack.back(); stack.pop_back();
    for (auto it = fs::directory_iterator(d, fs::directory_options::skip_permission_denied, ec);
         it != fs::directory_iterator(); it.increment(ec)) {
      if (cancel && cancel()) break;
      const fs::path& p = it->path();
      const auto name = QString::fromStdString(p.filename().u8string());
      if (name == "." || name == ".." || ignoreNames.contains(name)) continue;
      std::error_code ec2;
      if (it->is_directory(ec2)) {
        stack.emplace_back(p);
      } else if (it->is_regular_file(ec2)) {
        auto rel = QString::fromStdString(fs::relative(p, rootp, ec2).u8string());
        out.insert(rel, metaFromPath(p));
      }
    }
  }
  return out;
}

DiffItem compareFiles(const QString& src, const QString& dst) {
  fs::path sp = fs::u8path(src.toStdString());
  fs::path dp = fs::u8path(dst.toStdString());
  FileMeta s = metaFromPath(sp);
  FileMeta d = metaFromPath(dp);
  DiffItem di{QString::fromStdString(sp.filename().u8string()), Action::Identical, QString(), s, d};

  if (!s.exists && !d.exists) { di.reason = "Both missing"; return di; }
  if (s.exists && !d.exists) { di.action = Action::CopyNew; di.reason = "Missing in destination"; return di; }
  if (!s.exists && d.exists) { di.action = Action::OnlyInDest; di.reason = "Only in destination"; return di; }
  if (!(s.isFile && d.isFile)) { di.action = Action::TypeMismatch; di.reason = "Different type"; return di; }
  if (s.size == d.size && std::abs(s.mtime - d.mtime) <= MTIME_EPS) { di.reason = "Size/time match"; return di; }
  if (s.mtime > d.mtime + MTIME_EPS) { di.action = Action::CopyNewer; di.reason = "Source newer"; return di; }
  if (d.mtime > s.mtime + MTIME_EPS) { di.action = Action::SkipDestNewer; di.reason = "Destination newer"; return di; }
  di.action = Action::CopyMismatch; di.reason = "Ambiguous difference"; return di;
}

std::vector<DiffItem> compareDirs(const QString& srcRoot, const QString& dstRoot,
                                  const QSet<QString>& ignoreNames, const CancelFn& cancel) {
  auto srcFuture = QtConcurrent::run([&]{ return fastListFiles(srcRoot, ignoreNames, cancel); });
  auto dstFuture = QtConcurrent::run([&]{ return fastListFiles(dstRoot, ignoreNames, cancel); });
  MetaMap sm = srcFuture.result();
  MetaMap dm = dstFuture.result();

  // union of keys
  QSet<QString> keys;
  for (auto it = sm.constBegin(); it != sm.constEnd(); ++it) keys.insert(it.key());
  for (auto it = dm.constBegin(); it != dm.constEnd(); ++it) keys.insert(it.key());

  std::vector<DiffItem> diffs; diffs.reserve(keys.size());
  for (const auto& r : keys) {
    if (cancel && cancel()) break;
    fs::path sp = fs::u8path((srcRoot + "/" + r).toStdString());
    fs::path dp = fs::u8path((dstRoot + "/" + r).toStdString());

    const bool inS = sm.contains(r);
    const bool inD = dm.contains(r);
    if (inS && !inD) {
      diffs.push_back(DiffItem{r, Action::CopyNew, "Missing in dest", sm.value(r), FileMeta{}});
    } else if (!inS && inD) {
      diffs.push_back(DiffItem{r, Action::OnlyInDest, "Only in dest", FileMeta{}, dm.value(r)});
    } else {
      auto di = compareFiles(QString::fromStdString(sp.u8string()), QString::fromStdString(dp.u8string()));
      di.relpath = r;
      diffs.push_back(std::move(di));
    }
  }
  return diffs;
}

static bool ensureDirFor(const fs::path& p) {
  std::error_code ec; fs::create_directories(p.parent_path(), ec); return !ec;
}

bool copyItems(const QString& srcRoot, const QString& dstRoot, const std::vector<DiffItem>& diffs,
               int& copied, QStringList& errors, const CancelFn& cancel) {
  copied = 0; errors.clear();
  for (const auto& d : diffs) {
    if (cancel && cancel()) break;
    if (!(d.action == Action::CopyNew || d.action == Action::CopyNewer || d.action == Action::CopyMismatch))
      continue;
    std::filesystem::path sp = std::filesystem::u8path((srcRoot + "/" + d.relpath).toStdString());
    std::filesystem::path dp = std::filesystem::u8path((dstRoot + "/" + d.relpath).toStdString());
    std::error_code ec;
    ensureDirFor(dp);
    std::filesystem::copy_file(sp, dp, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
      errors << QString("%1: %2").arg(d.relpath, QString::fromStdString(ec.message()));
    } else {
      ++copied;
    }
  }
  return errors.isEmpty();
}

} // namespace aequalis
