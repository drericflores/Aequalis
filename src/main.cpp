
#include <QApplication>
#include "Aequalis/MainWindow.hpp"

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  aequalis::MainWindow w; w.resize(1200, 750); w.show();
  return app.exec();
}
