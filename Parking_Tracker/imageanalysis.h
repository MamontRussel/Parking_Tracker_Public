#ifndef IMAGEANALYSIS_H
#define IMAGEANALYSIS_H

#include <core/core.hpp>
#include <imgproc/imgproc.hpp>
#include <highgui/highgui.hpp>
#include <nonfree/nonfree.hpp>
#include <legacy/legacy.hpp>
#include <opencv.hpp>
#include <math.h>
#include <QTime>
#include <time.h>
#include <QFIle>

using namespace std;
using namespace cv;

const double eps = 1e-7;

class ImageAnalysis : public QObject
{
    Q_OBJECT

public:
    ImageAnalysis();
    void LoadData(CvPoint Reg[100][4], IplImage* nw, int num);
    void AnalysisImage(IplImage* nw);
signals:
    void ZoneStatus(int);
private:
    CvPoint ParkRegionCoord[100][4];// координаты парковочных мест
    IplImage* prev;
    IplImage* Ref[10];
    int ZoneNum;
    QString CurTime();
    double HistogramCompare(IplImage* n,IplImage* p);
    int64 HashCompare(IplImage* n,IplImage* p);
    int ImageDiff(IplImage* n,IplImage* p);
    void WriteLog(int index);
    void MakeDecision(int64 hash,double hist,int diff,int index);
    bool PointInPolygon(CvPoint t,int k);
    IplImage* maskImage(IplImage* rf,int i);
    int64 calcImageHash(IplImage* image);//расчет хэша
    int64 calcHammingDistance(int64 x, int64 y);
};

#endif // IMAGEANALYSIS_H
