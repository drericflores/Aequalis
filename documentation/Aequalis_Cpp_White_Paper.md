# White Paper — Aequalis (C++, Modular GUI)
**Version:** 1.0 (C++17 / Qt6, Qt5 fallback)  
**Author:** Dr. Eric Oliver Flores  
**Date:** 2025-10-14  
**Purpose:** A modular C++ port of Aequalis delivering the same semantics as the Python app with stronger performance and clean separation of concerns.

## 1) Executive Summary
The C++ port restructures Aequalis into a testable, maintainable architecture, using **Qt Widgets**, **std::filesystem** for high-performance traversal, and **Qt Concurrent** for parallel directory scans. The UI includes a **status bar progress bar**, **menu bar** (File/Tool/Help), and a two-tab **About** dialog. It preserves the original synchronization policy.

## 2) Functional Requirements (Parity with Python)
- **Modes:** Files vs Folders.  
- **Two-Pane Browsing:** Source | Destination with `QFileSystemModel`.  
- **Start at Home:** Defaults to the user’s home directory for pickers and views.  
- **Assessment & Policy:** Same as Python v1.2 (copy source new/newer; skip destination newer; notify only-in-dest).  
- **Actions:** Compare, Update Destination, Cancel (dialogs).  
- **Feedback:** Status text (“Scanning…”, “Comparing…”) + **progress bar** (indeterminate → determinate) + completion dialog.  
- **About:** Two tabs—(1) title/author/date/version; (2) technology stack and “Python 3 → C++” port note.

## 3) Modular Architecture
**Project Layout**
- `CMakeLists.txt` — `AUTOMOC/AUTOUIC/AUTORCC` enabled; Qt6 preferred, Qt5 fallback; links `Core/Widgets/Concurrent`.
- `include/Aequalis/`
  - `Types.hpp` — `FileMeta`, `DiffItem`, `Action`, `MTIME_EPS`.
  - `Scanner.hpp` — API for `fastListFiles`, `compareFiles`, `compareDirs`, `copyItems`.
  - `DiffModel.hpp` — `QAbstractTableModel` for results.
  - `Worker.hpp` — `CompareWorker` (`QThread`) with **signals**: `phase`, `progressRange`, `progressValue`, `done`, `failed`.
  - `MainWindow.hpp` — UI controller.
- `src/`
  - `Scanner.cpp` — std::filesystem traversal; comparison logic; copy with overwrite for allowed actions.
  - `DiffModel.cpp` — 7 columns: relpath, action, reason, src/dst mtime, src/dst size.
  - `Worker.cpp` — concurrent scanning using `QtConcurrent::run`; determines progress range after union of keys; emits determinate progress while comparing.
  - `MainWindow.cpp` — UI, menu bar, status bar + **progress bar**, ignore toggles, wiring of worker signals, About dialog.
  - `main.cpp` — application bootstrap.

**Key Design Points**
- **Parallel Scanning:** Source and Destination trees are enumerated simultaneously via `QtConcurrent::run`, reducing wall-clock time on large hierarchies.
- **Deterministic Progress:** After scans, we compute the union of relpaths to set a **finite progress range**; each compared file increments the progress bar.
- **Ignore Heavy Folders:** Optional filter (`.git`, `.hg`, `.svn`, `.idea`, `.vscode`, `node_modules`, `__pycache__`, `dist`, `build`) to reduce I/O.
- **Heuristic Compare:** (size, mtime±epsilon) keeps performance high while satisfying sync policy.
- **Safety:** Destination-newer files remain untouched.

## 4) Build & Deployment
**Dependencies:** Qt6 (or Qt5) Widgets/Concurrent; a C++17 compiler with `<filesystem>`.

**Ubuntu/Pop!_OS example**
```bash
sudo apt update
sudo apt install cmake build-essential qt6-base-dev
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
./aequalis
```

## 5) Development Challenges & Resolutions
- **Incomplete type errors (QTreeView/QWidget):**  
  Solved by including **proper Qt headers** in `MainWindow.cpp` (not just forward declarations) and ensuring all UI construction is in the cpp where full types are visible.
- **Undefined reference / vtable / signals/slots:**  
  Resolved by enabling **CMake AUTOMOC** and placing `Q_OBJECT` in QObject subclasses (`DiffModel`, `CompareWorker`, `MainWindow`), linking `Qt::Core/Widgets/Concurrent`.
- **Time-skew warnings on build:** benign; fixed by touching sources or rebuilding.
- **Feature parity:** added menu bar and **About** with 2 tabs; status bar **progress bar** (right-docked); same sync policy.

## 6) Performance & UX
- **Faster than Python** due to compiled C++ and `std::filesystem` + concurrent scanning.  
- **UI Responsiveness:** compare runs in a dedicated worker; **status + progress bar** reflect phase and throughput.  
- **Clear Assessment:** Same summarization (copy/newer-dst/only-dst/identical) and confirmation prompts as Python.

## 7) Future Enhancements
- **Content hashing (opt-in):** for identical-size/mtime ambiguity (e.g., `CopyMismatch`)—compute quick rolling hash or xxHash on demand.  
- **Copy progress/cancel:** mirrored worker with progress signals for the copy stage.  
- **Rulesets:** per-extension rules (e.g., always copy `.cfg` if source differs).  
- **Permissions/metadata:** optional extended attribute/ACL handling per platform.

## Version
**Aequalis (C++): v1.0 (Qt6/Qt5, C++17)**.
