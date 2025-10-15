
#include "Aequalis/MainWindow.hpp"
#include "Aequalis/Worker.hpp"
#include "Aequalis/Scanner.hpp"
#include <QFileDialog>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QTableView>
#include <QHeaderView>
#include <QFileSystemModel>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QLineEdit>
#include <QDir>
#include <QTreeView>
#include <QMenuBar>
#include <QStatusBar>
#include <QProgressBar>
#include <QTabWidget>
#include <QDialog>
#include <QTextBrowser>

namespace aequalis {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  buildUi();
  buildMenus();
}

void MainWindow::buildMenus() {
  auto* fileMenu = menuBar()->addMenu("&File");
  auto* actQuit  = fileMenu->addAction("Quit");
  connect(actQuit, &QAction::triggered, this, &QWidget::close);

  auto* toolMenu = menuBar()->addMenu("&Tool");
  auto* actCompare = toolMenu->addAction("Compare");
  connect(actCompare, &QAction::triggered, this, &MainWindow::doCompare);

  auto* helpMenu = menuBar()->addMenu("&Help");
  auto* actAbout = helpMenu->addAction("About");
  connect(actAbout, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::buildUi() {
  m_home = QDir::homePath();

  m_rbFolders = new QRadioButton("Folders");
  m_rbFiles   = new QRadioButton("Files");
  m_rbFolders->setChecked(true);

  m_cbSkipHeavy = new QCheckBox("Skip VCS/build folders (.git, node_modules, build, dist, __pycache__)");
  m_cbSkipHeavy->setChecked(true);

  m_srcEdit = new QLineEdit(m_home);
  m_dstEdit = new QLineEdit(m_home);
  m_srcBtn = new QPushButton("Browse…");
  m_dstBtn = new QPushButton("Browse…");

  connect(m_srcBtn, &QPushButton::clicked, this, &MainWindow::pickSource);
  connect(m_dstBtn, &QPushButton::clicked, this, &MainWindow::pickDestination);

  m_srcModel = new QFileSystemModel(this); m_srcModel->setRootPath(m_home);
  m_dstModel = new QFileSystemModel(this); m_dstModel->setRootPath(m_home);
  m_srcView = new QTreeView; m_srcView->setModel(m_srcModel); m_srcView->setRootIndex(m_srcModel->index(m_home));
  m_dstView = new QTreeView; m_dstView->setModel(m_dstModel); m_dstView->setRootIndex(m_dstModel->index(m_home));
  for (auto* v : {m_srcView, m_dstView}) { v->setAlternatingRowColors(true); v->setSortingEnabled(true); }

  auto* splitter = new QSplitter(Qt::Horizontal);
  auto* g1 = new QGroupBox("Source"); auto* l1 = new QVBoxLayout(g1); l1->addWidget(m_srcView);
  auto* g2 = new QGroupBox("Destination"); auto* l2 = new QVBoxLayout(g2); l2->addWidget(m_dstView);
  splitter->addWidget(g1); splitter->addWidget(g2);

  m_model = new DiffModel(this);
  m_table = new QTableView; m_table->setModel(m_model); m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

  m_compareBtn = new QPushButton("Compare");
  m_updateBtn  = new QPushButton("Update Destination");
  connect(m_compareBtn, &QPushButton::clicked, this, &MainWindow::doCompare);
  connect(m_updateBtn,  &QPushButton::clicked, this, &MainWindow::doUpdate);

  auto* form = new QGridLayout; int row = 0;
  auto* modeBox = new QWidget; auto* modeL = new QHBoxLayout(modeBox); modeL->setContentsMargins(0,0,0,0);
  modeL->addWidget(m_rbFolders); modeL->addWidget(m_rbFiles); modeL->addStretch(1);
  form->addWidget(new QLabel("Mode"), row, 0); form->addWidget(modeBox, row++, 1);
  auto addRow = [&](const QString& lbl, QLineEdit* edit, QPushButton* btn){
    auto* w = new QWidget; auto* hl = new QHBoxLayout(w); hl->setContentsMargins(0,0,0,0); hl->addWidget(edit); hl->addWidget(btn);
    form->addWidget(new QLabel(lbl), row, 0); form->addWidget(w, row++, 1);
  };
  addRow("Source", m_srcEdit, m_srcBtn);
  addRow("Destination", m_dstEdit, m_dstBtn);
  form->addWidget(m_cbSkipHeavy, row++, 1);

  auto* center = new QWidget; setCentralWidget(center);
  auto* root = new QVBoxLayout(center);
  root->addLayout(form);
  root->addWidget(splitter);
  root->addWidget(new QLabel("Assessment:"));
  root->addWidget(m_table);
  auto* ctl = new QHBoxLayout; ctl->addStretch(1); ctl->addWidget(m_compareBtn); ctl->addWidget(m_updateBtn);
  root->addLayout(ctl);

  // Status bar with message + progress bar on the right
  m_status = new QLabel("Ready");
  statusBar()->addWidget(m_status, 1);
  m_prog = new QProgressBar; m_prog->setMaximumWidth(260); m_prog->setTextVisible(true);
  m_prog->setRange(0, 1); m_prog->setValue(0); // idle
  statusBar()->addPermanentWidget(m_prog);
}

QSet<QString> MainWindow::currentIgnores() const {
  if (!m_cbSkipHeavy->isChecked()) return {};
  return QSet<QString>{".git", ".hg", ".svn", ".idea", ".vscode", "node_modules", "__pycache__", "dist", "build"};
}

void MainWindow::pickSource() {
  QString path;
  if (m_rbFolders->isChecked()) path = QFileDialog::getExistingDirectory(this, "Select Source Folder", m_home);
  else path = QFileDialog::getOpenFileName(this, "Select Source File", m_home);
  if (!path.isEmpty()) m_srcEdit->setText(path);
}

void MainWindow::pickDestination() {
  QString path;
  if (m_rbFolders->isChecked()) path = QFileDialog::getExistingDirectory(this, "Select Destination Folder", m_home);
  else path = QFileDialog::getOpenFileName(this, "Select Destination File", m_home);
  if (!path.isEmpty()) m_dstEdit->setText(path);
}

void MainWindow::doCompare() {
  const auto s = m_srcEdit->text().trimmed();
  const auto d = m_dstEdit->text().trimmed();
  if (s.isEmpty() || d.isEmpty()) { QMessageBox::warning(this, "Missing Paths", "Select both Source and Destination"); return; }

  m_status->setText("Scanning…");
  m_prog->setRange(0,0); // indeterminate during scan
  m_compareBtn->setEnabled(false); m_updateBtn->setEnabled(false);

  auto* w = new CompareWorker(m_rbFiles->isChecked(), s, d, currentIgnores(), this);
  connect(w, &CompareWorker::done, this, &MainWindow::onCompared);
  connect(w, &CompareWorker::failed, this, &MainWindow::onCompareFailed);
  connect(w, &CompareWorker::phase,  this, &MainWindow::onPhase);
  connect(w, &CompareWorker::progressRange, this, &MainWindow::onProgressRange);
  connect(w, &CompareWorker::progressValue, this, &MainWindow::onProgressValue);
  connect(w, &QThread::finished, w, &QObject::deleteLater);
  w->start();
}

void MainWindow::onPhase(const QString& phase) {
  m_status->setText(phase);
}

void MainWindow::onProgressRange(int min, int max) {
  m_prog->setRange(min, max);
  if (max==0) m_prog->setTextVisible(false); else m_prog->setTextVisible(true);
}

void MainWindow::onProgressValue(int val) {
  m_prog->setValue(val);
}

void MainWindow::onCompared(std::vector<DiffItem> diffs) {
  m_model->setDiffs(std::move(diffs));
  const auto& v = m_model->diffs();
  int copies=0,newer=0,onlyd=0,ident=0,typem=0;
  for (const auto& d : v) {
    switch (d.action) {
      case Action::CopyNew: case Action::CopyNewer: case Action::CopyMismatch: ++copies; break;
      case Action::SkipDestNewer: ++newer; break;
      case Action::OnlyInDest: ++onlyd; break;
      case Action::Identical: ++ident; break;
      case Action::TypeMismatch: ++typem; break;
    }
  }
  m_status->setText(QString("Compared %1 — copy:%2 newer-dst:%3 only-dst:%4 identical:%5 type-m:%6")
                    .arg(v.size()).arg(copies).arg(newer).arg(onlyd).arg(ident).arg(typem));
  m_prog->setRange(0,1); m_prog->setValue(0); // idle
  QMessageBox::information(this, "Assessment complete",
    QString("Compared %1 items.\\n\\n")
      .arg(v.size()) +
    QString("Will COPY: %1\\n").arg(copies) +
    QString("Destination NEWER (skipped): %1\\n").arg(newer) +
    QString("Only in Destination: %1\\n").arg(onlyd) +
    QString("Identical: %1").arg(ident)
  );
  m_compareBtn->setEnabled(true); m_updateBtn->setEnabled(true);
}

void MainWindow::onCompareFailed(QString err) {
  m_status->setText("Error");
  m_prog->setRange(0,1); m_prog->setValue(0);
  QMessageBox::critical(this, "Compare Error", err);
  m_compareBtn->setEnabled(true); m_updateBtn->setEnabled(true);
}

void MainWindow::doUpdate() {
  const auto s = m_srcEdit->text().trimmed();
  const auto d = m_dstEdit->text().trimmed();
  const auto diffs = m_model->diffs();
  int copies=0; QStringList errors;
  if (diffs.empty()) { QMessageBox::information(this, "Nothing to do", "Run Compare first."); return; }
  for (const auto& di : diffs) {
    if (di.action==Action::CopyNew || di.action==Action::CopyNewer || di.action==Action::CopyMismatch) { ++copies; }
  }
  if (copies==0) { QMessageBox::information(this, "Up-to-date", "No eligible items to copy."); return; }
  if (QMessageBox::question(this, "Confirm Update", QString("Copy %1 item(s)?").arg(copies)) != QMessageBox::Yes) return;
  int copied = 0;
  copyItems(s, d, diffs, copied, errors);
  if (!errors.isEmpty()) {
    QMessageBox::warning(this, "Completed with errors", QString("Copied %1; errors %2\\n").arg(copied).arg(errors.size()) + errors.join("\\n"));
  } else {
    QMessageBox::information(this, "Update complete", QString("Copied %1 items.").arg(copied));
    doCompare();
  }
}

void MainWindow::showAbout() {
  QDialog dlg(this);
  dlg.setWindowTitle("About Aequalis");
  auto* tabs = new QTabWidget(&dlg);

  // Tab 1: meta
  auto* meta = new QWidget; auto* ml = new QVBoxLayout(meta);
  auto* tb1 = new QTextBrowser; tb1->setHtml(
    "<h3>Aequalis – Compare & Sync</h3>"
    "<p><b>Author:</b> Dr. Eric Oliver Flores</p>"
    "<p><b>Date:</b> 2025-10-14</p>"
    "<p><b>Version:</b> 1.0 (C++ port)</p>");
  ml->addWidget(tb1); tabs->addTab(meta, "About");

  // Tab 2: technologies
  auto* tech = new QWidget; auto* tl = new QVBoxLayout(tech);
  auto* tb2 = new QTextBrowser; tb2->setHtml(
    "<h3>Technologies</h3>"
    "<ul>"
    "<li>Original implementation in <b>Python 3</b> (PyQt5)</li>"
    "<li>Ported to <b>C++17</b> with <b>Qt6/Qt5 Widgets</b></li>"
    "<li><b>std::filesystem</b> for fast traversal</li>"
    "<li><b>Qt Concurrent</b> for parallel scanning</li>"
    "<li><b>QThread</b> worker and signals/slots</li>"
    "<li><b>QAbstractTableModel</b> view-model for diff table</li>"
    "</ul>");
  tl->addWidget(tb2); tabs->addTab(tech, "Tech");

  auto* layout = new QVBoxLayout(&dlg);
  layout->addWidget(tabs);
  dlg.resize(520, 380);
  dlg.exec();
}

} // namespace aequalis
