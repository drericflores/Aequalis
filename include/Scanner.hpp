
#ifndef AEQUALIS_SCANNER_HPP
#define AEQUALIS_SCANNER_HPP
#include "Aequalis/Types.hpp"
#include <QString>
#include <QSet>
#include <QHash>
#include <QStringList>
#include <functional>
#include <vector>

namespace aequalis {

using CancelFn = std::function<bool()>;
using MetaMap = QHash<QString, FileMeta>; // relpath -> meta

// Collect regular files under root quickly; ignores names set.
MetaMap fastListFiles(const QString& root,
                      const QSet<QString>& ignoreNames = {},
                      const CancelFn& cancel = {});

DiffItem compareFiles(const QString& src, const QString& dst);

std::vector<DiffItem> compareDirs(const QString& srcRoot,
                                  const QString& dstRoot,
                                  const QSet<QString>& ignoreNames = {},
                                  const CancelFn& cancel = {});

bool copyItems(const QString& srcRoot,
               const QString& dstRoot,
               const std::vector<DiffItem>& diffs,
               int& copied,
               QStringList& errors,
               const CancelFn& cancel = {});

} // namespace aequalis

#endif // AEQUALIS_SCANNER_HPP
