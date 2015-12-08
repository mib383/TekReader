#include "headers/mainwindow.h"
#include "ui_mainwindow.h"

#include <stdio.h>
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
    sets.setValue(QString("keys/") + model.item(topLeft.row(),2)->text(),topLeft.data().toString());
    calcALL();
}

void MainWindow::setButtonIcon(int frame)
{
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

    //ui->btnSearch->setMovie();

    ui->btnRead->setIcon(QPixmap(":/icons/floppy.png"));

    //myMovie = new QMovie(":/icons/animatedFloppy.gif");
    myMovie = new QMovie(":/icons/anim.gif");

    //qDebug() << "fc:" << myMovie->frameCount();

    connect(myMovie,SIGNAL(frameChanged(int)),this,SLOT(setButtonIcon(int)));

    QString path = sets.value("path").toString();
    ui->lblPathDisplay->setText(path);

    calcALL();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addendDev(QString desc)
{
    QStandardItem* i1 = new QStandardItem(true);
    i1->setCheckable(true);
    i1->setCheckState(Qt::Checked);
    i1->setEditable(false);
    //i1->setFlags(Qt::ItemFlags);
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
    int h,m,n;
    int allnum;
    //bool flag = false;
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
                //full_str = s;
            }
        }

    }

    //if(flag)
    ALLnum = max;

    if(max > -1)
        ui->lblALL->setText(QString("ALL%1 (%2:%3)").arg(ALLnum,4,10,QChar('0')).arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0')));
    else
        ui->lblALL->setText("-");

    //return max;
    //QString path = path + "/" + name + "/ALL" + QString("%1").arg(allNum, 4, 10, QChar('0')) + "_" + datetime;
}

bool MainWindow::search()
{
    ViSession	rm = VI_NULL, vi = VI_NULL;
    ViStatus	status;
    ViChar		desc[256], id[256], buffer[256];
    ViUInt32	retCnt, itemCnt;
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
        addendDev(desc);

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

    //addendDev("abcd");
    //addendDev("efgh");
    return false;
}

void MainWindow::searchSlot()
{
    search();
}

void MainWindow::readSlot()
{
    //ui->btnRead->setDisabled(true);
    myMovie->start();
    read();
    //for(int i = 0; i < 4000000000; i++)
    //    if(i%100000000 == 0)
    //        QApplication::processEvents();

    myMovie->stop();
    ui->btnRead->setIcon(QPixmap(":/icons/floppy.png"));
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
            for(int j = 1; j < 5; j++)
            {
                if(ReadWaveform(vi, j, str))
                    WriteWaveform(ui->lblPathDisplay->text(), str, datetime, j);
                QApplication::processEvents();
            }

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

bool MainWindow::ReadWaveform(ViSession vi, int channel, QString name)
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
        res -= yoffset;
        res *= ymult;
        data.append(res);
        iv++;
    }
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

    QFile file(path + "/ch" + QString::number(ch) + ".csv");
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
        log("Unable to create file! File not saved!" + path + "/ch" + QString::number(ch) + ".csv", QColor("red"));
        return false;
    }
}
