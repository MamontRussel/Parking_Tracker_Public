#include "mainwindow.h"
#include <QDebug>

using namespace std;
using namespace cv;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    ui.setupUi(this);
    timer = new QTimer(this);
    timer->setInterval(33);
    connect(timer,SIGNAL(timeout()),this,SLOT(updateFrame()));
    Key=0;
    ZoneNum=0;
    all=false;//показывать все зоны
    track=false;//сначало настройка
    frame_count=0;
    setupMenu();
}
void MainWindow::about()
{
    QMessageBox::about(this,"Parking Tracker System","Parking Tracker System\nVer. 1.0.0\nDevelop by Denis Volkov and Vadim Begunov\n2015");
}

void MainWindow::saveZones()
{
    QString filePath = QFileDialog::getSaveFileName(this, tr("Выберите файл для сохранения конфигурации"),
                                                    "./configuration.bin", tr("Бинарный (*.bin);;Текстовый документ (*.txt)"));
    if (filePath.isEmpty()) return;
    char* path = QStringToChar(filePath);

    FILE* fd = fopen(path, "wb");
    if(fd==NULL)qDebug()<<"Error writing zones\n";
    fwrite(ParkRegionCoord,1,sizeof(ParkRegionCoord),fd);
    fwrite(&ZoneNum,1,sizeof(ZoneNum),fd);
    qDebug()<<"Succsess write\n";
    fclose(fd);
}

void MainWindow::loadZones()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Выберите файл для загрузки конфигурации"),
                                                    "./", tr("Бинарный (*.bin);;Текстовый документ (*.txt)"));
    if (filePath.isEmpty()) return;
    char* path = QStringToChar(filePath);

    FILE* fd = fopen(path, "rb");
    if(fd==NULL)qDebug()<<"Error reading zones\n";
    else
    {
        size_t result = fread(ParkRegionCoord,1,sizeof(ParkRegionCoord),fd);
        if(result!=sizeof(ParkRegionCoord))cout<<"Error size file\n";
        result = fread(&ZoneNum,1,sizeof(ZoneNum),fd);
        if(result!=sizeof(ZoneNum))cout<<"Error size file\n";
        QMainWindow::statusBar()->showMessage("Succsess read");
    }
    for(int i=0;i<ZoneNum;i++)
        ui.ZoneBox->addItem(QString::number(i+1));

    fclose(fd);
}

void MainWindow::startLiveTrack()
{
    capture = cvCreateCameraCapture(CV_CAP_ANY);
    assert(capture);

    InitVideo();
}

void MainWindow::startVideoTrack()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Выберите видеофайл"),"/Users/", tr("Avi (*.avi);;Mp4 (*.mp4)"));

    if (filePath.isEmpty()) return;

    char* path = QStringToChar(filePath);
    capture = cvCreateFileCapture(path);
    assert(capture);

    InitVideo();
 }

void MainWindow::setupMenu()
{
    QMenu* menuConf = new QMenu("&Конфигурация",this);
    menuConf->addAction("&About",this,SLOT(about()),Qt::CTRL + Qt::Key_W);
    menuConf->addAction("&Сохранить конфигурацию",this,SLOT(saveZones()));
    menuConf->addAction("&Загрузить конфигурацию",this,SLOT(loadZones()));

    QMenu* menuTrack = new QMenu("&Трекинг",this);
    menuTrack->addAction("&Live Трекинг",this,SLOT(startLiveTrack()));
    menuTrack->addAction("&Video Трекинг",this,SLOT(startVideoTrack()));

    QMainWindow::menuBar()->addMenu(menuTrack);
    QMainWindow::menuBar()->addMenu(menuConf);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    int x,y;
    if((event->pos().x()>=ui.label->pos().x() &&event->pos().x()<=ui.label->size().width()+ui.label->pos().x())&&( event->pos().y()>=ui.label->pos().y()-15 &&event->pos().y()<=ui.label->size().height()+ui.label->pos().y()+15))
       {
        x=event->pos().x()-12;
        y=event->pos().y()-25;

    switch (ParkRegionFillCheck())
    {
        case 4:
            ParkRegionCoord[Key][3].x=x;
            ParkRegionCoord[Key][3].y=y;
            ui.ZoneBox->addItem(QString::number(Key+1));
            ui.ZoneBox->setCurrentIndex(Key);
            ZoneNum++;
            break;
        case 3:
            ParkRegionCoord[Key][2].x=x;
            ParkRegionCoord[Key][2].y=y;
            break;
        case 2:
            ParkRegionCoord[Key][1].x=x;
            ParkRegionCoord[Key][1].y=y;
            break;
        case 1:
            ParkRegionCoord[Key][0].x=x;
            ParkRegionCoord[Key][0].y=y;
            break;
        default:
            break;
    }
    }
}

void MainWindow::keyPressEvent( QKeyEvent *k )
{
    int c=k->key();

    if( c<=57 && c>=48)//zone selection
    {
        Key=c-48;
    }
    if (c==65||c==97)//show all
    {
        if(all)all=false;
        else all=true;
    }
    if (c==68||c==100)//delete current zone
    {
        for(int j=0;j<4;j++)
        {
            ParkRegionCoord[Key][j].x=0;
            ParkRegionCoord[Key][j].y=0;
        }

    }

    if( c==27) //exit
        timer->stop();

}

void MainWindow::updateFrame()
{
    IplImage* frame;
    for(int i=0;i<speed;++i)
    {
        frame = cvQueryFrame(capture);
    }

    if(!frame)// если видеофайл кончился, выдать ошибку
    {
        timer->stop();
        QMessageBox::information(this, "Конец видео", "Видео файл закончился");
        track=false;
        return;
    }
    //custom resize, `cause video capture doesn`t support it. Error will be if resize Live Capture. Now both methods use this custom resize
    //Check performance for this thing
    if(frame->width!=720 || frame->height!=576)
    {
        IplImage *dest = cvCreateImage(cvSize(720,576),frame->depth,frame->nChannels);
        cvResize(frame,dest);
        frame=dest;
        //memory leak?
    }

    QImage imageView;

    char ParkZone[2];

    if(!track)
    {
    switch (ParkRegionFillCheck())
          {
                case 4:
                       //3 circles
                       cvCircle(frame, cvPoint(ParkRegionCoord[Key][2].x,ParkRegionCoord[Key][2].y), 2, CV_RGB(0,255,0),2,CV_AA,0);
                       cvCircle(frame, cvPoint(ParkRegionCoord[Key][1].x,ParkRegionCoord[Key][1].y), 2, CV_RGB(0,255,0),2,CV_AA,0);
                       cvCircle(frame, cvPoint(ParkRegionCoord[Key][0].x,ParkRegionCoord[Key][0].y), 2, CV_RGB(0,255,0),2,CV_AA,0);
                       break;
                   case 3:
                       cvCircle(frame, cvPoint(ParkRegionCoord[Key][1].x,ParkRegionCoord[Key][1].y), 2, CV_RGB(0,255,0),2,CV_AA,0);
                       cvCircle(frame, cvPoint(ParkRegionCoord[Key][0].x,ParkRegionCoord[Key][0].y), 2, CV_RGB(0,255,0),2,CV_AA,0);
                       //2 circles
                       break;
                   case 2:
                       cvCircle(frame, cvPoint(ParkRegionCoord[Key][0].x,ParkRegionCoord[Key][0].y), 2, CV_RGB(0,255,0),2,CV_AA,0);
                       //1 circle
                       break;
                   case -1:
                   {
                       if (!all)//Если не включен показ всех зон, рисуем текущую зону
                           DrawPolygon(Key,frame);
                       break;

                   }
                   default:
                       break;
               }
               snprintf(ParkZone,254,"%d",Key);//пишем рабочую зону
               cvPutText(frame, ParkZone, cvPoint(5,20), &font, CV_RGB(255,0,0));


               if(all)
                   for(int i=0;i<ZoneNum;i++)
                       DrawPolygon(i,frame);
            }
            else
    {
        frame_count++;

        snprintf(ParkZone,254,"%d",10-frame_count/30);//пишем рабочую зону
        cvPutText(frame, ParkZone, cvPoint(5,20), &font, CV_RGB(255,0,0));

        if(frame_count>=(30)*10)
        {
            ImA->AnalysisImage(frame);
            frame_count=0;
        }
        //draw all zones and mark empty one with green color
        for(int i=0;i<ZoneNum;i++)
            DrawPolygon(i,frame);

    }

   imageView = QImage((const unsigned char*)(frame->imageData), frame->width,frame->height,frame->widthStep,QImage::Format_RGB888).rgbSwapped();
   ui.label->setPixmap(QPixmap::fromImage(imageView));

   cvReleaseImage(&frame);
}

int MainWindow::ParkRegionFillCheck()
{
    if(ParkRegionCoord[Key][0].x!=0 && ParkRegionCoord[Key][0].y!=0 && ParkRegionCoord[Key][1].x!=0 && ParkRegionCoord[Key][1].y!=0 && ParkRegionCoord[Key][2].x!=0 && ParkRegionCoord[Key][2].y!=0 && ParkRegionCoord[Key][3].x==0 && ParkRegionCoord[Key][3].y==0)
        return 4;
    else if (ParkRegionCoord[Key][0].x!=0 && ParkRegionCoord[Key][0].y!=0 && ParkRegionCoord[Key][1].x!=0 && ParkRegionCoord[Key][1].y!=0 && ParkRegionCoord[Key][2].x==0 && ParkRegionCoord[Key][2].y==0 && ParkRegionCoord[Key][3].x==0 && ParkRegionCoord[Key][3].y==0)
        return 3;
    else if (ParkRegionCoord[Key][0].x!=0 && ParkRegionCoord[Key][0].y!=0 && ParkRegionCoord[Key][1].x==0 && ParkRegionCoord[Key][1].y==0 && ParkRegionCoord[Key][2].x==0 && ParkRegionCoord[Key][2].y==0 && ParkRegionCoord[Key][3].x==0 && ParkRegionCoord[Key][3].y==0)
        return 2;
    else if (ParkRegionCoord[Key][0].x==0 && ParkRegionCoord[Key][0].y==0 && ParkRegionCoord[Key][1].x==0 && ParkRegionCoord[Key][1].y==0 && ParkRegionCoord[Key][2].x==0 && ParkRegionCoord[Key][2].y==0 && ParkRegionCoord[Key][3].x==0 && ParkRegionCoord[Key][3].y==0)
        return 1;
    else if(ParkRegionCoord[Key][0].x!=0 && ParkRegionCoord[Key][0].y!=0 && ParkRegionCoord[Key][1].x!=0 && ParkRegionCoord[Key][1].y!=0 && ParkRegionCoord[Key][2].x!=0 && ParkRegionCoord[Key][2].y!=0 && ParkRegionCoord[Key][3].x!=0 && ParkRegionCoord[Key][3].y!=0)
        return -1;
    else return 0;
}

void MainWindow::ClearParkZone()
{
    for(int i=0;i<100;i++)
    {
        for(int j=0;j<4;j++)
        {
            ParkRegionCoord[i][j].x=0;
            ParkRegionCoord[i][j].y=0;
        }
        ZoneEmpty[i]=true;
    }
}

void MainWindow::DrawPolygon(int i,IplImage* frame)
{
    char ParkZone[5];

    CvScalar color;
    if(ZoneEmpty[i])
        color=CV_RGB(0,255,0);
    else
        color=CV_RGB(255,0,0);

    cvLine(frame, cvPoint(ParkRegionCoord[i][0].x,ParkRegionCoord[i][0].y), cvPoint(ParkRegionCoord[i][1].x,ParkRegionCoord[i][1].y), color,2,CV_AA,0);
    cvLine(frame, cvPoint(ParkRegionCoord[i][1].x,ParkRegionCoord[i][1].y), cvPoint(ParkRegionCoord[i][2].x,ParkRegionCoord[i][2].y), color,2,CV_AA,0);
    cvLine(frame, cvPoint(ParkRegionCoord[i][2].x,ParkRegionCoord[i][2].y), cvPoint(ParkRegionCoord[i][3].x,ParkRegionCoord[i][3].y), color,2,CV_AA,0);
    cvLine(frame, cvPoint(ParkRegionCoord[i][0].x,ParkRegionCoord[i][0].y), cvPoint(ParkRegionCoord[i][3].x,ParkRegionCoord[i][3].y), color,2,CV_AA,0);

    snprintf(ParkZone,254,"%d",i+1);//пишем зону
    cvPutText(frame, ParkZone, cvPoint(ParkRegionCoord[i][0].x+(ParkRegionCoord[i][2].x-ParkRegionCoord[i][0].x)/2, ParkRegionCoord[i][0].y+(ParkRegionCoord[i][2].y-ParkRegionCoord[i][0].y)/2), &font, CV_RGB(255,255,255));
}
void MainWindow::InitVideo()
{
    cvInitFont(&font, CV_FONT_HERSHEY_COMPLEX_SMALL, 1.0, 1.0, 1,1,8);

    QFile file("./zones.log");
    file.open(QIODevice::WriteOnly| QIODevice::Text);
    QTextStream out(&file);
    out<<"";
    file.close();

    ClearParkZone();
    timer->start();
}

void MainWindow::on_addZonebutton_clicked()
{
    if(ZoneNum>0)Key=ZoneNum-1;
    else Key=ZoneNum;
    if (ParkRegionFillCheck()==-1)Key++;
}

void MainWindow::on_ZoneBox_currentIndexChanged(int index)
{
    Key=index;
}

char* MainWindow::QStringToChar(QString str)
{
    QByteArray ba = str.toLatin1();
    return ba.data();
}

void MainWindow::on_pushButton_2_clicked()
{
    //check for at least one zone
    if(ui.ZoneBox->count()>0&&cvQueryFrame(capture)!=NULL)
    {
        ui.addZonebutton->setEnabled(false);
        ui.ZoneBox->setEnabled(false);
        ui.AllZoneCheck->setEnabled(false);
        track=true;
        ImA = new ImageAnalysis();
        timer->stop();
        ImA->LoadData(ParkRegionCoord,cvQueryFrame(capture),ZoneNum);
        connect(ImA,SIGNAL(ZoneStatus(int)),this,SLOT(ZoneUp(int)));
        timer->start();// make sure that LoadData complete before update frame
    }
    else
    {
        QMessageBox::information(this, "Нет зон", "Не добавлено ни одной зоны или нет видео потока.Трекинг не возможен.");
    }
}

void MainWindow::on_AllZoneCheck_clicked()
{
    all=ui.AllZoneCheck->isChecked();
}

void MainWindow::on_pushButton_clicked()
{
    timer->stop();
}

void MainWindow::on_pushButton_3_clicked()
{
    timer->start();
}

void MainWindow::ZoneUp(int index)
{
    if(ZoneEmpty[index])ZoneEmpty[index]=false;
    else ZoneEmpty[index]=true;
}

void MainWindow::on_pushButton_4_clicked()
{
    speed=1;
}

void MainWindow::on_pushButton_5_clicked()
{
    speed=2;
}

void MainWindow::on_pushButton_6_clicked()
{
    speed=4;
}

void MainWindow::on_pushButton_7_clicked()
{
    speed=16;
}
