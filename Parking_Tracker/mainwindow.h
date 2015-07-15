#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"
#include "imageanalysis.h"
#include <highgui/highgui.hpp>
#include <core/core.hpp>
#include <opencv.hpp>
#include <QTimer>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QMouseEvent>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
public slots:
    void ZoneUp(int);
private slots:
    void updateFrame();
    void about();
    void saveZones();
    void loadZones();
    void startLiveTrack();
    void startVideoTrack();
    void on_addZonebutton_clicked();
    void on_ZoneBox_currentIndexChanged(int index);
    void on_pushButton_2_clicked();
    void on_AllZoneCheck_clicked();
    void on_pushButton_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
    void on_pushButton_6_clicked();
    void on_pushButton_7_clicked();

private:
    Ui::MainWindow ui;
    void InitVideo();
    int Key; //храним выбранный регион
    int ZoneNum,frame_count,speed=1;
    bool ZoneEmpty[100];
    CvPoint ParkRegionCoord[100][4];// координаты парковочных мест
    CvFont font; //шрифт
    int ParkRegionFillCheck();
    void ClearParkZone();
    void DrawPolygon(int i,IplImage* frame);
    void mousePressEvent(QMouseEvent *event);
    void keyPressEvent( QKeyEvent *k );
    void setupMenu();
    QTimer* timer;
    bool all,track;
    char* QStringToChar(QString str);
    CvCapture* capture;
    ImageAnalysis* ImA;
};

#endif // MAINWINDOW_H
