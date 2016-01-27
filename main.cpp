/*!
  \file
  \author mib383
  \brief This program is designed to reading data from multiple Tektronix oscilloscopes.

 */

#include "headers/mainwindow.h"
#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile file(":styles/darkorange.css");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    a.setStyleSheet(styleSheet);

    MainWindow w;

    w.show();

    return a.exec();
}
