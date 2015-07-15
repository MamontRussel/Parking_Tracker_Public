#include "imageanalysis.h"
#include <QDebug>
#include <QTextStream>

ImageAnalysis::ImageAnalysis()
{

}

void ImageAnalysis::LoadData(CvPoint Reg[100][4],IplImage* nw,int num)
{
    for(int i=0;i<100;i++)
        for(int j=0;j<4;j++)
            ParkRegionCoord[i][j]=Reg[i][j];

    prev=cvCreateImage(cvSize(720,576),nw->depth,nw->nChannels);
    IplImage *dest = cvCreateImage(cvSize(720,576),nw->depth,nw->nChannels);
    cvResize(nw,dest);
    nw=dest;
    cvCopy(nw,prev);
    ZoneNum=num;
    cvReleaseImage(&dest);
}

double area(CvPoint a, CvPoint b, CvPoint c)
 {
     return 0.5 * (a.x * b.y + b.x * c.y + c.x * a.y - a.y * b.x - b.y * c.x - c.y * a.x);
 }

bool ccw (CvPoint a, CvPoint b, CvPoint c)
 {
         return area (a, b, c) > eps;
 }

CvPoint min_py(CvPoint a,CvPoint b)
{
    return a.y < b.y || (abs (a.y - b.y) <= eps && a.x < b.x) ? a : b;
}

CvPoint max_py(CvPoint a,CvPoint b)
{
   return a.y > b.y || (abs (a.y - b.y) <= eps && a.x > b.x) ? a : b;
}

bool ImageAnalysis::PointInPolygon(CvPoint t,int k)
{
    int i,j;
    int count=0;
    for(i=0;i<4;i++)
    {
        j=(i+1)%4;
        if(min(ParkRegionCoord[k][i].y,ParkRegionCoord[k][j].y)<t.y&&t.y<=max(ParkRegionCoord[k][i].y,ParkRegionCoord[k][j].y)&&ccw(min_py (ParkRegionCoord[k][i], ParkRegionCoord[k][j]), max_py (ParkRegionCoord[k][i], ParkRegionCoord[k][j]), t))

            {
                count++;
            }
    }

    return count % 2;
}

IplImage* ImageAnalysis::maskImage(IplImage* rf,int i)
{
    for( int y=0; y<rf->height; y++ )
    {
        uchar* ptr = (uchar*) (rf->imageData + y * rf->widthStep);
        for( int x=0; x<rf->width; x++ )
        {
            if(!PointInPolygon(cvPoint(x,y),i))
            {
                ptr[3*x] = 0;     // B - синий
                ptr[3*x+1] = 0;   // G - зелёный
                ptr[3*x+2] = 0; // R - красный
            }
        }
    }
    return rf;
}

void ImageAnalysis::AnalysisImage(IplImage *nw)
{
    double minx,maxy,maxx,miny,time;
    IplImage* rf=cvCreateImage(cvSize(720,576),nw->depth,nw->nChannels);
    IplImage* prevrf=cvCreateImage(cvSize(720,576),prev->depth,prev->nChannels);
    cvCopy(nw,rf);
    cvCopy(prev,prevrf);
    time=clock();
    for(int i=0;i<ZoneNum;i++)
    {
        minx=fmin(fmin(ParkRegionCoord[i][0].x,ParkRegionCoord[i][1].x),fmin(ParkRegionCoord[i][2].x,ParkRegionCoord[i][3].x));
        maxy=fmax(fmax(ParkRegionCoord[i][0].y,ParkRegionCoord[i][1].y),fmax(ParkRegionCoord[i][2].y,ParkRegionCoord[i][3].y));
        miny=fmin(fmin(ParkRegionCoord[i][0].y,ParkRegionCoord[i][1].y),fmin(ParkRegionCoord[i][2].y,ParkRegionCoord[i][3].y));
        maxx=fmax(fmax(ParkRegionCoord[i][0].x,ParkRegionCoord[i][1].x),fmax(ParkRegionCoord[i][2].x,ParkRegionCoord[i][3].x));
        cvSetImageROI(rf,cvRect(minx,miny,maxx-minx,maxy-miny));
        cvSetImageROI(prevrf,cvRect(minx,miny,maxx-minx,maxy-miny));

        rf=maskImage(rf,i);
        prevrf=maskImage(prevrf,i);
        HashCompare(rf,prevrf);
        MakeDecision(HashCompare(rf,prevrf),HistogramCompare(rf,prevrf),ImageDiff(rf,prevrf),i);

        cvResetImageROI(prevrf);
        cvResetImageROI(rf);
        cvCopy(nw,rf);
        cvCopy(prev,prevrf);
    }
    cvReleaseImage(&rf);
    cvReleaseImage(&prevrf);
    cvCopy(nw,prev);//болван, незабывай, что в opencv копируется указатель, если не делать cvCopy
    qDebug()<<"Time to compute:"<<(clock()-time)/CLOCKS_PER_SEC;
}

double ImageAnalysis::HistogramCompare(IplImage* n, IplImage *p)
{
    //Сравнение гистограмм

     Mat nw(n);
     Mat pr(p);
     Mat hvs_nw,hvs_pr;

     /// Convert to HSV
     cvtColor( nw, hvs_nw, COLOR_BGR2HSV );
     cvtColor( pr, hvs_pr, COLOR_BGR2HSV );

     int h_bins = 50; int s_bins = 60;
     int histSize[] = { h_bins, s_bins };

     // hue varies from 0 to 179, saturation from 0 to 255
     float h_ranges[] = { 0, 180 };
     float s_ranges[] = { 0, 256 };

     const float* ranges[] = { h_ranges, s_ranges };

     // Use the o-th and 1-st channels
     int channels[] = { 0, 1 };

     MatND hist_nw;
     MatND hist_pr;

     /// Calculate the histograms for the HSV images
     calcHist( &hvs_nw, 1, channels, Mat(), hist_nw, 2, histSize, ranges, true, false );
     normalize( hist_nw, hist_nw, 0, 1, NORM_MINMAX, -1, Mat() );

     calcHist( &hvs_pr, 1, channels, Mat(), hist_pr, 2, histSize, ranges, true, false );
     normalize( hist_pr, hist_pr, 0, 1, NORM_MINMAX, -1, Mat() );

     double percent=compareHist( hist_nw, hist_pr, 0);//using Correlation method
     nw.release();
     pr.release();
     hist_nw.release();
     hist_pr.release();
     hvs_nw.release();
     hvs_pr.release();

     return percent;
}

QString ImageAnalysis::CurTime()
{
    QTime clock;
    return QString::number(clock.currentTime().hour())+":"+QString::number(clock.currentTime().minute())+":"+QString::number(clock.currentTime().second());
}

int64 ImageAnalysis::HashCompare(IplImage *n, IplImage *p)
{
    //сравнение хэшем

    //построим хэш
    int64 hashO = calcImageHash(n);
    int64 hashI = calcImageHash(p);

    //рассчитаем расстояние Хэмминга
    return calcHammingDistance(hashO, hashI);
}

int64 ImageAnalysis::calcImageHash(IplImage* src)
{
         if(!src)
         {
                return 0;
         }
            IplImage *res=0, *gray=0, *bin =0;

            res = cvCreateImage( cvSize(8, 8), src->depth, src->nChannels);
            gray = cvCreateImage( cvSize(8, 8), IPL_DEPTH_8U, 1);
            bin = cvCreateImage( cvSize(8, 8), IPL_DEPTH_8U, 1);

            // уменьшаем картинку
            cvResize(src, res);
            // переводим в градации серого
            cvCvtColor(res, gray, CV_BGR2GRAY);
            // вычисляем среднее
            CvScalar average = cvAvg(gray);

            // получим бинарное изображение относительно среднего
            // для этого воспользуемся пороговым преобразованием
            cvThreshold(gray, bin, average.val[0], 255, CV_THRESH_BINARY);
            // построим хэш
            int64 hash = 0;

            int i=0;
            // пробегаемся по всем пикселям изображения
            for( int y=0; y<bin->height; y++ ) {
                    uchar* ptr = (uchar*) (bin->imageData + y * bin->widthStep);
                    for( int x=0; x<bin->width; x++ ) {
                            // 1 канал
                            if(ptr[x])
                            {
                                    // hash |= 1<<i;  // warning C4334: '<<' : result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
                                    hash |= 1<<i;
                            }
                            i++;
                    }
            }

    // освобождаем ресурсы
    cvReleaseImage(&res);
    cvReleaseImage(&gray);
    cvReleaseImage(&bin);
    return hash;
}

// рассчёт расстояния Хэмминга между двумя хэшами
// http://en.wikipedia.org/wiki/Hamming_distance
// http://ru.wikipedia.org/wiki/Расстояние_Хэмминга
//
int64 ImageAnalysis::calcHammingDistance(int64 x, int64 y)
{
    int64 dist = 0, val = x ^ y;

    // Count the number of set bits
    while(val)
    {
       ++dist;
       val &= val - 1;
    }

    return dist;
}

int ImageAnalysis::ImageDiff(IplImage* n, IplImage *p)
{

    IplImage *n_gry,*prev_gry,*diff;

    n_gry=cvCreateImage(cvGetSize(n),IPL_DEPTH_8U,1);
    prev_gry=cvCreateImage(cvGetSize(p),IPL_DEPTH_8U,1);
    diff=cvCreateImage(cvGetSize(n),IPL_DEPTH_8U,1);

    cvCvtColor(n,n_gry,CV_RGB2GRAY);
    cvCvtColor(p,prev_gry,CV_RGB2GRAY);

    cvAbsDiff(prev_gry,n_gry,diff);
    cvThreshold(diff,diff,45,255,CV_THRESH_TOZERO);

    int percent=(cvCountNonZero(diff)*100)/(diff->height*diff->width);//calculate percent

    cvReleaseImage(&n_gry);
    cvReleaseImage(&prev_gry);
    cvReleaseImage(&diff);

    return percent;
}

void ImageAnalysis::MakeDecision(int64 hash, double hist, int diff, int index)
{
    qDebug()<<hash<<hist<<diff;

        qDebug()<<"Zone:"<<index+1;
        if((hash>0&&hist<0.9&&diff>0)||(hash>0&&diff>=15)||(diff>=30))
        {
            emit ZoneStatus(index);
            qDebug()<<"Update Zone State"<<CurTime();
            WriteLog(index);
        }
}

void ImageAnalysis::WriteLog(int index)
{

    QFile file("./zones.log");
    file.open(QIODevice::ReadWrite| QIODevice::Text);
    QString a;
    QTextStream in(&file);
    a=in.readAll();
    a=QString::number(index)+" "+CurTime()+"\n";
    QTextStream out(&file);
    out<<a;
    file.close();
}

