
#ifndef AEQUALIS_MAINWINDOW_HPP
#define AEQUALIS_MAINWINDOW_HPP
#include <QMainWindow>
#include <QSet>

QT_BEGIN_NAMESPACE
class QLineEdit; class QPushButton; class QRadioButton; class QLabel; class QTreeView; class QFileSystemModel; class QTableView; class QCheckBox; class QProgressBar; class QMenu;
QT_END_NAMESPACE

#include "Aequalis/DiffModel.hpp"

namespace aequalis {

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(QWidget* parent=nullptr);

private slots:
  void pickSource();
  void pickDestination();
  void doCompare();
  void onCompared(std::vector<DiffItem> diffs);
  void onCompareFailed(QString err);
  void doUpdate();
  void showAbout();

  // worker progress
  void onPhase(const QString& phase);
  void onProgressRange(int min, int max);
  void onProgressValue(int val);

private:
  void buildUi();
  void buildMenus();
  QSet<QString> currentIgnores() const;

  QString m_home;
  QWidget* m_central{nullptr};
  QRadioButton* m_rbFolders{nullptr};
  QRadioButton* m_rbFiles{nullptr};
  QLineEdit* m_srcEdit{nullptr};
  QLineEdit* m_dstEdit{nullptr};
  QPushButton* m_srcBtn{nullptr};
  QPushButton* m_dstBtn{nullptr};
  QPushButton* m_compareBtn{nullptr};
  QPushButton* m_updateBtn{nullptr};
  QLabel* m_status{nullptr};
  QProgressBar* m_prog{nullptr};
  QTreeView* m_srcView{nullptr};
  QTreeView* m_dstView{nullptr};
  QFileSystemModel* m_srcModel{nullptr};
  QFileSystemModel* m_dstModel{nullptr};
  QTableView* m_table{nullptr};
  DiffModel* m_model{nullptr};
  QCheckBox* m_cbSkipHeavy{nullptr};
};

} // namespace aequalis

#endif // AEQUALIS_MAINWINDOW_HPP
