#include "headers/mainwindow.h"
#include "ui_mainwindow.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "headers/visa.h"
#include <QDebug>
#include <QTime>
#include <QVector>
#include <assert.h>

#include <QFileDialog>
#include <QDir>
#include <QStyle>
#include <QStandardItemModel>
#include <QList>

void MainWindow::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    Q_UNUSED(bottomRight);
    sets.setValue(QString("keys/") + model.item(topLeft.row(),2)->text(),topLeft.data().toString());
    calcALL();
}

void MainWindow::setButtonIcon(int frame)
{
    Q_UNUSED(frame);
    ui->btnRead->setIcon(QIcon(myMovie->currentPixmap()));
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    sets("mib383","TekReader"),
    data(0),
    model(0,3,0)
{
    ui->setupUi(this);
    QStringList headerLabels;
    headerLabels << "?" << "Name" << "Description";
    model.setHorizontalHeaderLabels(headerLabels);

    ui->devTable->setModel(&model);

    ui->devTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->devTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->devTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    QFont font = ui->devTable->horizontalHeader()->font();
    font.setPointSize( 14 );
    ui->devTable->horizontalHeader()->setFont( font );

    connect(ui->devTable->model(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(onDataChanged(const QModelIndex&, const QModelIndex&)));

    ui->btnBrowse->setIcon(QPixmap(":/icons/folder_blue.png"));
    ui->btnSearch->setIcon(QPixmap(":/icons/search.png"));


    ui->btnRead->setIcon(QPixmap(":/icons/floppy.png"));
    ui->btnRead->setStyleSheet("QPushButton{background-color: QLinearGradient( x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #f0b7a1, stop: 0.5 #8c3310 , stop: 1 #f0b7a1)}");


    myMovie = new QMovie(":/icons/anim.gif");


    connect(myMovie,SIGNAL(frameChanged(int)),this,SLOT(setButtonIcon(int)));

    QString path = sets.value("path").toString();
    ui->lblPathDisplay->setText(path);

    calcALL();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::appendDev(QString desc)
{
    QStandardItem* i1 = new QStandardItem(true);
    i1->setCheckable(true);
    i1->setCheckState(Qt::Checked);
    i1->setEditable(false);

    QStandardItem* i2 = new QStandardItem(sets.value(QString("keys/") + QString(desc)).toString());
    QStandardItem* i3 = new QStandardItem(desc);
    i3->setEditable(false);
    QList<QStandardItem*>* lst = new QList<QStandardItem*>();
    lst->append(i1);
    lst->append(i2);
    lst->append(i3);
    model.appendRow(*lst);
    calcALL();
}

void MainWindow::calcALL()
{
    QString desc, name;

    int max = -1;
    int h,m;
    int allnum;

    for(int i = 0; i < model.rowCount(); i++)
    {
        desc = model.item(i, 2)->text();
        name = model.item(i, 1)->text();

        if(name != "")
        {
            desc = name;
        }

        desc = desc.replace(QRegExp("[^a-zA-Z0-9]"),"_");

        QDir qdir(ui->lblPathDisplay->text() + "/" + desc);

        QString f = "ALL*";
        QStringList sl = qdir.entryList(QStringList(f));


        foreach(QString s, sl)
        {
            //flag = true;
            QString tmp;
            tmp = s.mid(3,4);
            allnum = tmp.toInt();
            if(max < allnum)
            {
                max = allnum;

                tmp = s.mid(17,2);
                h = tmp.toInt();

                tmp = s.mid(19,2);
                m = tmp.toInt();
            }
        }
    }


    ALLnum = max;

    if(max > -1)
        ui->lblALL->setText(QString("ALL%1 (%2:%3)").arg(ALLnum,4,10,QChar('0')).arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0')));
    else
        ui->lblALL->setText("-");
    ALLnum++;
}
#ifndef QT_NO_DEBUG
bool MainWindow::dSearch()
{
    ViChar		desc[256];
    ViUInt32	itemCnt;
    ViUInt32	i;

    model.removeRows(0, model.rowCount());
    itemCnt = 5;


    for (i = 0; i < itemCnt; i++)
    {
        sprintf(desc, "dev::%lu", i);
        appendDev(desc);
    }
    return true;
}
#endif

bool MainWindow::search()
{
    ViSession	rm = VI_NULL, vi = VI_NULL;
    ViStatus	status;
    ViChar		desc[256], id[256], buffer[256];
    ViUInt32	retCnt, itemCnt = 0;
    ViFindList	list;
    ViUInt32	i;

    model.removeRows(0, model.rowCount());
    // Open a default Session
    status = viOpenDefaultRM(&rm);
    if (status < VI_SUCCESS) goto error1;
    // Find all GPIB devices
    status = viFindRsrc(rm, (ViString)"USB?*INSTR", &list, &itemCnt, desc);
    if (status < VI_SUCCESS) goto error1;

    for (i = 0; i < itemCnt; i++)
    {
        status = viOpen(rm, desc, VI_NULL, VI_NULL, &vi);
        if (status <  VI_SUCCESS) goto error1;

        status = viSetAttribute(vi, VI_ATTR_TMO_VALUE, 3000);

        status = viWrite(vi, (ViBuf) "*idn?", 5, &retCnt);
        if (status < VI_SUCCESS) goto error1;

        // Clear the buffer and read the response
        memset(id, 0, sizeof(id));
        status = viRead(vi, (ViBuf) id, sizeof(id), &retCnt);
        if (status < VI_SUCCESS) goto error1;

        //log(QString("Found: ") + desc + "; " + id, QColor("green"));
        appendDev(desc);

        viClose(vi);

        viFindNext(list, desc);
    }
    viClose(rm);
    return true;
error1:
    viStatusDesc(vi, status, buffer);
    log(QString("Failure: ") + buffer, QColor("red"));

    if (rm != VI_NULL) {
        viClose(rm);
    }
    return false;
}

void MainWindow::searchSlot()
{
#ifdef QT_NO_DEBUG
    search();
#else
    dSearch();
#endif
    //appendDev("123");
}

void MainWindow::readSlot()
{
    myMovie->start();
#ifdef QT_NO_DEBUG
    read();
   #else
    dRead();
#endif
    myMovie->stop();
    ui->btnRead->setIcon(QPixmap(":/icons/floppy.png"));
    calcALL();
}

void MainWindow::browseClick()
{
    QString cur_path = sets.value("path").toString();
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                    cur_path,
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    if(dir != QString::null)
    {
        ui->lblPathDisplay->setText(dir);
        sets.setValue("path", dir);
        calcALL();
    }
}

void MainWindow::log(QString s, QColor c)
{
    ui->txtLog->moveCursor (QTextCursor::End);
    QString str = QString("<font color=\"") + c.name(QColor::HexArgb) + "\">[" + QTime::currentTime().toString() + "] " + s + "</color> <br>";
    ui->txtLog->insertHtml (str);
}

bool MainWindow::dRead()
{
    ViSession	vi = VI_NULL;
    ViChar		desc[256];
    ViUInt32	itemCnt;
    ViUInt32	i;

    itemCnt = 5;

    calcALL();

    for (i = 0; i < itemCnt; i++)
    {
        sprintf(desc, "dev::%lu", i);

        if(model.item(model.findItems(QString(desc),Qt::MatchExactly,2)[0]->row(),0)->checkState() == Qt::Checked)//i2 > -1 && devList.CheckedStatus[i2] == Qt::Checked)
        {
            QString datetime = QDateTime::currentDateTime().toString("yyyyMMdd_hhmm");
            QString str;
            if(sets.value(QString("keys/") + desc).toString() != "")
            {
                str = sets.value(QString("keys/") + desc).toString();
            }
            else
            {
                str = QString(desc);
            }
            //log(QString("     Reading data from ") + str + "...", QColor("green"));
            // for each channel...
            QVector<float> chan[4];
            QVector<float> offsets(4);
            QVector<float> mults(4);
            QVector<bool> flags(4);
            float SR;
            for(int j = 1; j < 5; j++)
            {
                if(dReadWaveform(vi, j, str, chan[j-1],offsets[j-1],mults[j-1],SR))
                {
                    flags[j - 1] = true;
                    WriteWaveform(ui->lblPathDisplay->text(), str, datetime, j);
                }
                else
                {
                    flags[j-1] = false;
                }

                QApplication::processEvents();
            }
            MakeGnuplot(ui->lblPathDisplay->text(), str, flags, chan, offsets, mults, SR, datetime);
        }
    }
    return true;
}

bool MainWindow::read()
{
    ViSession	rm = VI_NULL, vi = VI_NULL;
    ViStatus	status;
    ViChar		desc[256], id[256], buffer[256];
    ViUInt32	retCnt, itemCnt;
    ViFindList	list;
    ViUInt32	i;



    // Open a default Session
    status = viOpenDefaultRM(&rm);
    if (status < VI_SUCCESS) goto error;

    // Find all USB devices
    status = viFindRsrc(rm, (ViString)"USB?*INSTR", &list, &itemCnt, desc);
    if (status < VI_SUCCESS) goto error;

    calcALL();

    for (i = 0; i < itemCnt; i++)
    {
        // Open resource found in rsrc list
        status = viOpen(rm, desc, VI_NULL, VI_NULL, &vi);
        if (status <  VI_SUCCESS) goto error;

        status = viSetAttribute(vi, VI_ATTR_TMO_VALUE, 3000);

        // Send an ID query.
        status = viWrite(vi, (ViBuf) "*idn?", 5, &retCnt);
        if (status < VI_SUCCESS) goto error;

        // Clear the buffer and read the response
        memset(id, 0, sizeof(id));
        status = viRead(vi, (ViBuf) id, sizeof(id), &retCnt);
        if (status < VI_SUCCESS) goto error;

        //int i2 = model.findItems(QString(desc))[0]->row(); //map.keys().indexOf(QString(desc));

        //QString d = QString(desc);
        //QList<QStandardItem*> its = model.findItems(d,Qt::MatchExactly,2);

        //int row = its[0]->row();

        //QStandardItem* it = model.item(row,0);

        if(model.item(model.findItems(QString(desc),Qt::MatchExactly,2)[0]->row(),0)->checkState() == Qt::Checked)//i2 > -1 && devList.CheckedStatus[i2] == Qt::Checked)
        {
            QString datetime = QDateTime::currentDateTime().toString("yyyyMMdd_hhmm");
            QString str;
            if(sets.value(QString("keys/") + desc).toString() != "")
            {
                str = sets.value(QString("keys/") + desc).toString();
            }
            else
            {
                str = QString(desc);
            }
            //log(QString("     Reading data from ") + str + "...", QColor("green"));
            // for each channel...
            QVector<float> chan[4];
            QVector<float> offsets(4);
            QVector<float> mults(4);
            QVector<bool> flags(4);
            float SR;
            for(int j = 1; j < 5; j++)
            {
                if(ReadWaveform(vi, j, str, chan[j-1],offsets[j-1],mults[j-1],SR))
                {
                    flags[j - 1] = true;
                    WriteWaveform(ui->lblPathDisplay->text(), str, datetime, j);
                }
                else
                {
                    flags[j-1] = false;
                }

                QApplication::processEvents();
            }
            MakeGnuplot(ui->lblPathDisplay->text(), str, flags, chan, offsets, mults, SR,datetime);
        }

        // We're done with this device so close it
        viClose(vi);

        // Get the next item
        viFindNext(list, desc);
    }

    // Clean up
    viClose(rm);


    //devList.setMyStringList(sl);
    return true;

error:
    // Report error and clean up
    viStatusDesc(vi, status, buffer);
    log(QString("Failure: ") + buffer, QColor("red"));
    //fprintf(stderr, "failure: %s\n", buffer);
    if (rm != VI_NULL) {
        viClose(rm);
    }
    //devList.setMyStringList(sl);
    return false;
}

bool MainWindow::MakeGnuplot(QString path, QString name, QVector<bool> flags, QVector<float> vecs[4], QVector<float> offsets, QVector<float> mults, float sr,QString datetime)
{
    name = name.replace(QRegExp("[^a-zA-Z0-9]"),"_");
    QDir qdir(path + "/" + name);
    path = path + "/" + name;// + "/ALL" + QString("%1").arg(ALLnum, 4, 10, QChar('0')) + "_" + datetime;
    if(!qdir.mkpath(path))
    {
        path = QApplication::applicationDirPath() + "/" + name;// + "/ALL" + QString("%1").arg(ALLnum, 4, 10, QChar('0')) + "_" + datetime;
        log("Unable to create directory. Trying to save file in app folder...", QColor("yellow"));
        if(!qdir.mkpath(path))
        {
            log("Unable to create directory in app folder! File not saved!", QColor("red"));
            return false;
        }
    }
    QString NAME = path + "/"+name + "_all" + QString("%1").arg(ALLnum, 4, 10, QChar('0')) + ".dat";
    QString FigName = path + "/"+name +"_all" + QString("%1").arg(ALLnum, 4, 10, QChar('0')) + ".png";
    QFile file(NAME);
    if ( file.open(QIODevice::ReadWrite) )
    {
        QTextStream stream(&file);

        for(int i = 0; i < 2500; i++)
        {
            stream << QString("%1").arg(sr * i) << "\t"
                   << (flags[0]?QString("%1").arg(vecs[0][i]/* - offsets[0]*/):0) << "\t"
                   << (flags[0]?QString("%1").arg(vecs[1][i]/* - offsets[1]*/):0) << "\t"
                   << (flags[0]?QString("%1").arg(vecs[2][i]/* - offsets[2]*/):0) << "\t"
                   << (flags[0]?QString("%1").arg(vecs[3][i]/* - offsets[3]*/):0) << "\t"
                                                    << endl;
        }
        file.close();
        QString gnuplot_cmd;
        /*NAME = path+"/all0000.dat";
        sr = 4e-8;
        mults[0] = 0.008;
        mults[1] = 0.004;
        mults[2] = 0.0002;
        mults[3] = 0.02;*/
        gnuplot_cmd = QString("gnuplot -persist -e \"set term png background rgb 'black'; set output \\\"")
                    + FigName
                    + "\\\"; "
                    + "set border lw 2 lc rgb 'white'; "
                    + "set grid xtics ytics ls 3 lw 1 lc rgb 'gray'; "
                    + "set key outside center bottom Left reverse box lw 2 lc rgb 'white' maxcols 2 maxrows 2 textcolor rgb 'white'; "
                    + "set xlabel \\\"" + toStringSuffix(sr*250) + "s\\\" textcolor rgb 'white';"
                    + "set xrange [0:" + QString("%1").arg(sr*2500) + "];"
                    + "set ylabel \\\"" + "" + "\\\" textcolor rgb 'white';"
                    + "set yrange [-4:4];"
                    + "set xtics " + QString("%1").arg(sr*250) + " textcolor rgb 'white'; set xtics format \\\"%.1te%.1S\\\";"
                    + "set ytics 1; set ytics format \\\" \\\";"
                    + " set title \\\""+ name + "\\nall" + QString("%1 \\ndate/time:").arg(ALLnum, 4, 10, QChar('0')) + datetime/*QString("%1").arg(sr)*/ + "\\\" noenhanced textcolor rgb 'white';"
                    + " plot \\\""
                    + NAME
                    + "\\\" using 1:($2/25.6) title \\\"ch1: " + toStringSuffix(mults[0]*25)+ "V\\\" with lines lw 2 lc rgb 'yellow',\\\""
                    + NAME
                    + "\\\" using 1:($3/25.6) title \\\"ch2: " + toStringSuffix(mults[1]*25)+ "V\\\" with lines lw 2 lc rgb 'cyan',\\\""
                    + NAME
                    + "\\\" using 1:($4/25.6) title \\\"ch3: " + toStringSuffix(mults[2]*25)+ "V\\\" with lines lw 2 lc rgb 'dark-violet',\\\""
                    + NAME
                    + "\\\" using 1:($5/25.6) title \\\"ch4: " + toStringSuffix(mults[3]*25)+ "V\\\" with lines lw 2 lc rgb 'green'\"";
        //qDebug() << gnuplot_cmd << "\n";
        system(gnuplot_cmd.toStdString().c_str());
        //log(QString("File for ") + name + " (channel " + QString::number(ch) + ") saved successfully", QColor("green"));
        return true;
    }
    else
    {
        log("Unable to create file! File not saved! " + NAME + ".dat", QColor("red"));
        return false;
    }
}

QString MainWindow::toStringSuffix(float x)
{
    if(x >= 1e-9 && x < 1e-6)
        return QString("%1n").arg(x*1e9,0,'f',1);
    if(x >= 1e-6 && x < 1e-3)
        return QString("%1u").arg(x*1e6,0,'f',1);
    if(x >= 1e-3 && x < 1e0)
        return QString("%1m").arg(x*1e3,0,'f',1);
    if(x >= 1e0 && x < 1e3)
        return QString("%1").arg(x*1e0,0,'f',1);
    if(x >= 1e3 && x < 1e6)
        return QString("%1k").arg(x*1e-3,0,'f',1);
    return QString("0");
}

bool MainWindow::dReadWaveform(ViSession vi, int channel, QString name, QVector<float>& all_chan, float& OFFSET, float& MULT, float& SR)
{
    Q_UNUSED(vi);
    Q_UNUSED(name);
    float		yoffset, ymult, horscale, sampling_rate;
    long reclen;

    int iv = 0; // счетчик для вектора
    int jv = 0; // счетчик для считанного буфера

    ViUInt32 cnt;
    ymult = channel * 10;

    yoffset = channel * 10;


    horscale = channel * 10;
    reclen = 2500;
    sampling_rate = horscale * 10 / reclen;
    cnt = 2506;
    jv+=6;// пропускаем служебную информацию
    data.clear();
    float res;
    for(ViUInt32 k = jv; k < cnt; k++)
    {
        data.append(sampling_rate * iv);
        res = (signed char)(rand()%256 - 128);//buf[k];
        all_chan.append(res);
        res -= yoffset;
        res *= ymult;
        data.append(res);
        iv++;
    }
    OFFSET = yoffset;
    MULT = ymult;
    SR = sampling_rate;
    return true;
}

bool MainWindow::ReadWaveform(ViSession vi, int channel, QString name, QVector<float>& all_chan, float& OFFSET, float& MULT, float& SR)
{
    ViStatus	status;
    float		yoffset, ymult, horscale, sampling_rate;
    ViChar		buffer[256];
   // ViChar      ds[256];
    long reclen;

    int iv = 0; // счетчик для вектора
    int jv = 0; // счетчик для считанного буфера
    ViByte buf[3000]; // берем с запасом

    ViUInt32 cnt;
    QVector<float> vec(0);
    int j=100000;

    status = viSetAttribute(vi,VI_ATTR_WR_BUF_OPER_MODE, VI_FLUSH_ON_ACCESS);
    status = viSetAttribute(vi,VI_ATTR_RD_BUF_OPER_MODE, VI_FLUSH_ON_ACCESS);

    // Turn headers off, this makes parsing easier
    status = viPrintf(vi, (ViString)"header off\n");
    if (status < VI_SUCCESS) goto error;

    // set source as channel
    assert(channel > 0 && channel < 5);
    status  = viPrintf(vi, (ViString)"DATa:SOUrce CH%d\n", channel);
    if (status < VI_SUCCESS) goto error;

    //status = viQueryf(vi, (ViString)"CH1:YUNit?\n", (ViString)"%s", ds);
    //qDebug() << QString(ds) + "\n";
    //if (status < VI_SUCCESS) goto error;

    // Get record length value
    status = viQueryf(vi, (ViString)"hor:reco?\n", (ViString)"%ld", &reclen);
    if (status < VI_SUCCESS) goto error;

    // Make sure start, stop values for curve query match the full record length
    status = viPrintf(vi, (ViString)"data:start 1;data:stop %ld\n", reclen);
    if (status < VI_SUCCESS) goto error;

    // Get the yoffset to help calculate the vertical values.

    // Get the ymult to help calculate the vertical values.
    status = viQueryf(vi, (ViString)"WFMPre:YMULT?\n", (ViString)"%f", &ymult);
    if (status < VI_SUCCESS)
    {
        log(name + " channel " + QString::number(channel) + " is off", QColor("yellow"));
        return false;
    }


    status = viQueryf(vi, (ViString)"WFMPre:YOFF?\n", (ViString)"%f", &yoffset);
    if (status < VI_SUCCESS) goto error;



    // Request 8bit binary data on the curve query
    status = viPrintf(vi, (ViString)"DATA:ENCDG RIBINARY;WIDTH 1\n");
    if (status < VI_SUCCESS) goto error;

    // Requset for horizontal scale
    status = viQueryf(vi, (ViString)"HORIZONTAL:MAIN:SCALE?\n", (ViString)"%f", &horscale);
    if (status < VI_SUCCESS) goto error;
    sampling_rate = horscale * 10 / reclen;

    // Request the curve
    status = viPrintf(vi, (ViString)"CURVE?\n");
    if (status < VI_SUCCESS) goto error;

    //wait a little

    while(j--);


    status = viRead(vi, buf, 3000, &cnt);
    if (status < VI_SUCCESS) goto error;

    jv+=6;// пропускаем служебную информацию
    data.clear();
    float res;
    for(ViUInt32 k = jv; k < cnt; k++)
    {
        data.append(sampling_rate * iv);
        res = (signed char)buf[k];
        all_chan.append(res);
        res -= yoffset;
        res *= ymult;
        data.append(res);
        iv++;
    }
    OFFSET = yoffset;
    MULT = ymult;
    SR = sampling_rate;
    return true;

error:

    viStatusDesc(vi, status, buffer);
    log(QString("Failure: ") + buffer, QColor("red"));
    return false;
    //fprintf(stderr, "failure: %s\n", buffer);
    //if (ptr != NULL) free(ptr);
    //return vec;
}

bool MainWindow::WriteWaveform(QString path, QString name, QString datetime, int ch)
{
    //calcALL();
    name = name.replace(QRegExp("[^a-zA-Z0-9]"),"_");
    QDir qdir(path + "/" + name);
    path = path + "/" + name + "/ALL" + QString("%1").arg(ALLnum, 4, 10, QChar('0')) + "_" + datetime;



    if(!qdir.mkpath(path))
    {
        path = QApplication::applicationDirPath() + "/" + name + "/ALL" + QString("%1").arg(ALLnum, 4, 10, QChar('0')) + "_" + datetime;
        log("Unable to create directory. Trying to save file in app folder...", QColor("yellow"));
        if(!qdir.mkpath(path))
        {
            log("Unable to create directory in app folder! File not saved!", QColor("red"));
            return false;
        }
    }

    //QFile file(path + "/ch" + QString::number(ch) + ".csv");
    QFile file(path + "/" + name + "_" + "all" + QString("%1").arg(ALLnum, 4, 10, QChar('0')) + "_ch" + QString::number(ch) + ".csv");
    if ( file.open(QIODevice::ReadWrite) )
    {
        QTextStream stream(&file);
        for(int i = 0; i < 15; i++)
        {
            stream << endl;
        }

        int cnt = data.count();
        for(int i = 0; i < cnt - 2; i += 2)
        {
            stream << ",,," << QString("%1").arg(data[i]) << ", " << data[i+1] << "," << endl;
        }
        file.close();
        //log(QString("File for ") + name + " (channel " + QString::number(ch) + ") saved successfully", QColor("green"));
        return true;
    }
    else
    {
        log("Unable to create file! File not saved! " + path + "/ch" + QString::number(ch) + ".csv", QColor("red"));
        return false;
    }
}
