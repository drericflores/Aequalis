
#ifndef AEQUALIS_TYPES_HPP
#define AEQUALIS_TYPES_HPP
#include <QString>
#include <cstdint>
#include <string>

namespace aequalis {

static constexpr double MTIME_EPS = 1.0; // seconds tolerance

struct FileMeta {
  bool exists{false};
  bool isFile{false};
  std::uintmax_t size{0};
  double mtime{0.0}; // seconds since epoch
};

enum class Action {
  Identical,
  CopyNew,
  CopyNewer,
  CopyMismatch,
  SkipDestNewer,
  OnlyInDest,
  TypeMismatch
};

struct DiffItem {
  QString relpath;
  Action action{Action::Identical};
  QString reason;
  FileMeta src;
  FileMeta dst;
};

} // namespace aequalis

#endif // AEQUALIS_TYPES_HPP
