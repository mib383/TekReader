#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QMap>
#include "visa.h"
#include <QStandardItemModel>
#include <QMovie>

namespace Ui {
class MainWindow;
}

/*!
 * \brief The MainWindow class
 * Basic class for gui
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    QMovie* myMovie;
    Ui::MainWindow *ui;
    QSettings sets; ///<settings object to save app settings via executions
    QVector<float> data; ///<data array to store reading data
    QStandardItemModel model; ///<model for device table
    int ALLnum;///<var to store ALLXXXX number

    bool search();
    void log(QString s, QColor c);
    bool read();
    //bool ReadWaveform(ViSession vi, int channel, QString name);
    bool WriteWaveform(QString path, QString name, QString datetime, int ch);
    void appendDev(QString desc);
    void calcALL();
    bool ReadWaveform(ViSession vi, int channel, QString name, QVector &all_chan, float &OFFSET, float &MULT, float &SR);
public slots:
    void searchSlot();
    void readSlot();
    void browseClick();
    void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    void setButtonIcon(int frame);
};

#endif // MAINWINDOW_H
