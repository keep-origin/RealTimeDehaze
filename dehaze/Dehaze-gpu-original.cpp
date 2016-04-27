using namespace std;

#include <opencv2/opencv.hpp>
#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <deque>
#include <opencv2/core/cuda.hpp>
#include <cmath>
#include <iostream>
#include "opencv2/core.hpp"
#include <opencv2/core/utility.hpp>
#include "opencv2/imgproc.hpp"
#include "opencv2/cudaimgproc.hpp"
#include <opencv2/cudaarithm.hpp>

using namespace cv;

inline double maxnum(double a, double b)
{
	
	if (a >= b)
	{
		return a;
	}
	else
	{
		return b;
	}
}
cuda::GpuMat boxfilter(cuda::GpuMat im, int r)
{
	Mat imSrc(im);
	int hei = imSrc.rows;
	int wid = imSrc.cols;
	Mat imDst = Mat::zeros(imSrc.size(), imSrc.type());
	//cumulative sum over Y axis
	Mat imCum1 = Mat::zeros(imSrc.size(), imSrc.type());

	for (int j = 0; j<imSrc.cols; j++)
	{
		imCum1.at<double>(0, j) = imSrc.at<double>(0, j);
	}
	for (int i = 1; i<imSrc.rows; i++)
	{
		for (int j = 0; j<imSrc.cols; j++)
		{
			imCum1.at<double>(i, j) = imSrc.at<double>(i, j) + imCum1.at<double>(i - 1, j);
		}
	}
	//for (int i=0;i<imSrc.rows;i++)
	//{
	//	cout<<imCum1.at<double>(i,0)<<endl;
	//}
	for (int i = 0; i<r + 1; i++)
	{
		for (int j = 0; j<imSrc.cols; j++)
		{
			imDst.at<double>(i, j) = imCum1.at<double>(i + r, j);
		}
	}

	for (int i = r + 1; i<hei - r; i++)
	{
		for (int j = 0; j<imSrc.cols; j++)
		{
			imDst.at<double>(i, j) = imCum1.at<double>(i + r, j) - imCum1.at<double>(i - r - 1, j);
		}
	}

	for (int i = hei - r; i<hei; i++)
	{
		for (int j = 0; j<imSrc.cols; j++)
		{
			imDst.at<double>(i, j) = imCum1.at<double>(hei - 1, j) - imCum1.at<double>(i - r - 1, j);
		}
	}
	//for (int i=595;i<600;i++)
	//{
	//	cout<<imDst.at<double>(i,0)<<endl;
	//}

	//cumulative sum over X axis
	Mat imCum2 = Mat::zeros(imSrc.size(), imSrc.type());
	for (int i = 0; i<imSrc.rows; i++)
	{
		imCum2.at<double>(i, 0) = imDst.at<double>(i, 0);
	}
	for (int i = 0; i<imSrc.rows; i++)
	{
		for (int j = 1; j<imSrc.cols; j++)
		{
			imCum2.at<double>(i, j) = imDst.at<double>(i, j) + imCum2.at<double>(i, j - 1);
		}
	}

	for (int i = 0; i<imSrc.rows; i++)
	{
		for (int j = 0; j<r + 1; j++)
		{
			imDst.at<double>(i, j) = imCum2.at<double>(i, j + r);
		}
	}

	for (int i = 0; i<imSrc.rows; i++)
	{
		for (int j = r + 1; j<wid - r; j++)
		{
			imDst.at<double>(i, j) = imCum2.at<double>(i, j + r) - imCum2.at<double>(i, j - r - 1);
		}
	}

	for (int i = 0; i<imSrc.rows; i++)
	{
		for (int j = wid - r; j<wid; j++)
		{
			imDst.at<double>(i, j) = imCum2.at<double>(i, wid - 1) - imCum2.at<double>(i, j - r - 1);
		}
	}
	//for (int i=590;i<600;i++)
	//{
	//	for (int j=0;j<5;j++)
	//	{
	//		cout<<imDst.at<double>(i,j)<<" ";
	//	}
	//	cout<<endl;
	//	
	//}
	//cout<<endl;
	//double minval,maxval;
	//minMaxLoc(imDst,&minval,&maxval);
	//cout<<minval<<endl;
	//cout<<maxval<<endl;
	cuda::GpuMat dest(imDst);
	return dest;
}
cuda::GpuMat guidedfilter(Mat I, Mat p, int r, double eps)
{
	cuda::GpuMat Ig(I);
	cuda::GpuMat pg(p);
	
	int hei = I.rows;
	int wid = I.cols;

	Mat N = Mat::ones(I.size(), I.type());
	cuda::GpuMat Ng_temp(N);
	cuda::GpuMat Ng = boxfilter(Ng_temp, r);

	cuda::GpuMat aa = boxfilter(Ig, r);
	
	cuda::GpuMat mean_Ig;	;	
	cuda::divide(aa, Ng, mean_Ig);

	cuda::GpuMat mean_pg;
	cuda::divide(cuda::GpuMat(boxfilter(pg, r)), Ng, mean_pg);

	cuda::GpuMat mean_Ipg;

	//multiply(I, p, Ip);
	cuda::GpuMat Ipg;
	cuda::multiply(Ig, pg, Ipg);
	cuda::GpuMat temp = boxfilter(Ipg, r);	
	cuda::divide(temp, Ng, mean_Ipg);
	//divide(boxfilter(Ip, r), N, mean_Ip);


	//this is the covariance of (I, p) in each local patch.
	cuda::GpuMat mean_I_pg;
	cuda::multiply(mean_Ig, mean_pg, mean_I_pg);
	cuda::GpuMat cov_Ipg;
	cuda::subtract(mean_Ipg, mean_I_pg, cov_Ipg);

	cuda::GpuMat IIg;
	cuda::multiply(Ig, Ig, IIg);
	cuda::GpuMat mean_IIg;
	cuda::divide(cuda::GpuMat(boxfilter(IIg, r)), Ng, mean_IIg);
	
	cuda::GpuMat mean_I_Ig;
	cuda::multiply(mean_Ig, mean_Ig, mean_I_Ig);
	cuda::GpuMat var_Ig;
	cuda::subtract(mean_IIg, mean_I_Ig, var_Ig);

	// Eqn. (5) in the paper;
	cuda::GpuMat ag;
	var_Ig.convertTo(var_Ig, var_Ig.type(), 1.0, eps);	
	cuda::divide(cov_Ipg, var_Ig, ag);

	// Eqn. (6) in the paper;
	cuda::GpuMat a_mean_Ig;
	cuda::multiply(ag, mean_Ig, a_mean_Ig);
	cuda::GpuMat bg;
	cuda::subtract(mean_pg, a_mean_Ig, bg);
	
	double minval, maxval;
	cuda::minMaxLoc(bg, &minval, &maxval, NULL, NULL);

	cout << minval << endl;
	cout << maxval << endl;
	
	cuda::GpuMat mean_ag; 
	cuda::divide(cuda::GpuMat(boxfilter(ag, r)), Ng, mean_ag);
	cuda::GpuMat mean_bg;
	cuda::divide(cuda::GpuMat(boxfilter(bg, r)), Ng, mean_bg);


	//Eqn. (8) in the paper;		
	cuda::GpuMat qg;
	cuda::GpuMat mean_a_Ig;
	cuda::multiply(mean_ag, Ig, mean_a_Ig);
	cuda::add(mean_a_Ig, mean_bg, qg);
	
	return qg;

}
void MakeMapping(int* Histgram, float CutLimit = 0.01)
{
	int I, Sum = 0, Amount = 0;
	const int Level = 256;
	for (I = 0; I < Level; I++) Amount += Histgram[I];
	int MinB, MaxB;
	for (I = 0; I < Level; I++)
	{
		Sum = Sum + Histgram[I];
		if (Sum >= Amount * CutLimit)
		{
			MinB = I;
			break;
		}
	}
	Sum = 0;
	for (I = Level - 1; I >= 0; I--)
	{
		Sum = Sum + Histgram[I];
		if (Sum >= Amount * CutLimit)
		{
			MaxB = I;
			break;
		}
	}

	if (MaxB != MinB)
	{
		for (I = 0; I < Level; I++)
		{
			if (I<MinB)
				Histgram[I] = 0;
			else if (I>MaxB)
				Histgram[I] = 255;
			else
				Histgram[I] = 255 * (I - MinB) / (MaxB - MinB);
		}
	}
	else
	{
		for (I = 0; I < Level; I++)
			Histgram[I] = MaxB;        //     this  is must
	}
}

int block = 7;//5
double w1 = 200;//8
double w;
IplImage *src = NULL;
IplImage *dst = NULL;

IplImage* dehaze(IplImage *src, int block, double w)
{
	//rgb channels
	IplImage* dst1 = NULL;
	IplImage* dst2 = NULL;
	IplImage* dst3 = NULL;
	//dst ROI
	IplImage* imgroi1;
	IplImage* imgroi2;
	IplImage* imgroi3;

	IplImage* roidark;

	IplImage* dark_channel = NULL;

	IplImage* toushelv = NULL;
	//dehaze channels
	IplImage* j1 = NULL;
	IplImage* j2 = NULL;
	IplImage* j3 = NULL;
	//combine channels
	IplImage* dst = NULL;
	//image ROI
	CvRect ROI_rect;

	dst1 = cvCreateImage(cvSize(src->width, src->height), IPL_DEPTH_8U, 1);
	dst2 = cvCreateImage(cvSize(src->width, src->height), IPL_DEPTH_8U, 1);
	dst3 = cvCreateImage(cvSize(src->width, src->height), IPL_DEPTH_8U, 1);

	imgroi1 = cvCreateImage(cvSize(block, block), IPL_DEPTH_8U, 1);
	imgroi2 = cvCreateImage(cvSize(block, block), IPL_DEPTH_8U, 1);
	imgroi3 = cvCreateImage(cvSize(block, block), IPL_DEPTH_8U, 1);
	roidark = cvCreateImage(cvSize(block, block), IPL_DEPTH_8U, 1);

	j1 = cvCreateImage(cvSize(src->width, src->height), IPL_DEPTH_8U, 1);
	j2 = cvCreateImage(cvSize(src->width, src->height), IPL_DEPTH_8U, 1);
	j3 = cvCreateImage(cvSize(src->width, src->height), IPL_DEPTH_8U, 1);
	dark_channel = cvCreateImage(cvSize(src->width, src->height), IPL_DEPTH_8U, 1);

	toushelv = cvCreateImage(cvSize(src->width, src->height), IPL_DEPTH_8U, 1);

	dst = cvCreateImage(cvSize(src->width, src->height), IPL_DEPTH_8U, 3);

	cvSplit(src, dst1, dst2, dst3, NULL);

	ROI_rect.width = block;
	ROI_rect.height = block;
	ROI_rect.x = 0;
	ROI_rect.y = 0;

	int i, j;
	double min1 = 0;
	double max1 = 0;
	double min2 = 0;
	double max2 = 0;
	double min3 = 0;
	double max3 = 0;
	double min = 0;
	CvScalar value;
	for (i = 0; i<src->width / block; i++)
	{
		for (j = 0; j<src->height / block; j++)
		{
			cvSetImageROI(dst1, ROI_rect);
			cvCopy(dst1, imgroi1, NULL);
			cvMinMaxLoc(imgroi1, &min1, &max1, NULL, NULL);

			cvSetImageROI(dst2, ROI_rect);
			cvCopy(dst2, imgroi2, NULL);
			cvMinMaxLoc(imgroi2, &min2, &max2, NULL, NULL);

			cvSetImageROI(dst3, ROI_rect);
			cvCopy(dst3, imgroi3, NULL);
			cvMinMaxLoc(imgroi3, &min3, &max3, NULL, NULL);

			if (min1<min2)
			{
				min = min1;
			}
			else
			{
				min = min2;
			}
			if (min>min3)
			{
				min = min3;
			}
			//min is ROI Jdrak
			value = cvScalar(min, min, min, min);

			cvSetImageROI(dark_channel, ROI_rect);

			cvSet(roidark, value);
			//cout<<roidark->width<<endl;
			//cout<<roidark->height<<endl;


			cvCopy(roidark, dark_channel, NULL);

			cvResetImageROI(dst1);
			cvResetImageROI(dst2);
			cvResetImageROI(dst3);
			cvResetImageROI(dark_channel);

			ROI_rect.x = block*i;
			ROI_rect.y = block*j;


		}
	}

	cvSaveImage("dark_channel.bmp", dark_channel);

	double min_dark;
	double max_dark;

	CvPoint min_loc;
	CvPoint max_loc;
	cvMinMaxLoc(dark_channel, &min_dark, &max_dark, &min_loc, &max_loc, NULL);
	ROI_rect.x = max_loc.x;
	ROI_rect.y = max_loc.y;

	double A_dst1;
	double dst1_min;

	double A_dst2;
	double dst2_min;

	double A_dst3;
	double dst3_min;

	cvSetImageROI(dst1, ROI_rect);
	cvCopy(dst1, imgroi1, NULL);
	cvMinMaxLoc(imgroi1, &dst1_min, &A_dst1, NULL, NULL);

	cvSetImageROI(dst2, ROI_rect);
	cvCopy(dst2, imgroi2, NULL);
	cvMinMaxLoc(imgroi2, &dst2_min, &A_dst2, NULL, NULL);

	cvSetImageROI(dst3, ROI_rect);
	cvCopy(dst3, imgroi3, NULL);
	cvMinMaxLoc(imgroi3, &dst3_min, &A_dst3, NULL, NULL);

	int k, l;
	CvScalar m, n;
	for (k = 0; k<src->height; k++)
	{
		for (l = 0; l<src->width; l++)
		{
			m = cvGet2D(dark_channel, k, l);
			n = cvScalar(255 - w*m.val[0]);
			cvSet2D(toushelv, k, l, n);
		}
	}

	cvSaveImage("tousgelv.jpg", toushelv);

	int p, q;
	double tx;
	double jj1, jj2, jj3;
	CvScalar ix, jx;
	for (p = 0; p<src->height; p++)
	{
		for (q = 0; q<src->width; q++)
		{
			tx = cvGetReal2D(toushelv, p, q);
			tx = tx / 255;
			if (tx<0.1)
			{
				tx = 0.1;
			}
			ix = cvGet2D(src, p, q);
			jj1 = (ix.val[0] - A_dst1) / tx + A_dst1;
			jj2 = (ix.val[1] - A_dst2) / tx + A_dst2;
			jj3 = (ix.val[2] - A_dst3) / tx + A_dst3;
			jx = cvScalar(jj1, jj2, jj3, 0.0);
			cvSet2D(dst, p, q, jx);
		}
	}
	cvSaveImage("dehaze.bmp", dst);

	cvReleaseImage(&dst1);
	cvReleaseImage(&dst2);
	cvReleaseImage(&dst3);
	cvReleaseImage(&imgroi1);
	cvReleaseImage(&imgroi2);
	cvReleaseImage(&imgroi3);
	cvReleaseImage(&roidark);
	cvReleaseImage(&dark_channel);
	cvReleaseImage(&toushelv);
	cvReleaseImage(&j1);
	cvReleaseImage(&j2);
	cvReleaseImage(&j3);


	return dst;
}

Mat dehaze_cplusplus(Mat& img, double kenlRatio, int minAtomsLight)
{
	cout << "Entering dehaze" << endl;

	Mat imgcopy = img.clone();
	int w = img.cols;//width
	int h = img.rows;//height
	//cout<<img.channels()<<endl;
	//cout<<img.type()<<endl;

	/****************************************/
	Mat dc;
	dc = Mat::zeros(h, w, CV_8UC1);//dc stores min of channels
	
	//uchar minval;
	for (int i = 0; i<h; i++)
	{
		for (int j = 0; j<w; j++)
		{
			//if(img.at<Vec3b>(i,j)[0]>img.at<Vec3b>(i,j)[1])
			//	minval=img.at<Vec3b>(i,j)[1];
			//else
			//	minval=img.at<Vec3b>(i,j)[0];
			//if (minval>img.at<Vec3b>(i,j)[2])
			//{
			//	minval=img.at<Vec3b>(i,j)[2];
			//} 
			//
			//dc.at<uchar>(i,j)=minval;

			dc.at<uchar>(i, j) = (uchar)max((int)img.at<Vec3b>(i, j)[0], max((int)img.at<Vec3b>(i, j)[1], (int)img.at<Vec3b>(i, j)[2]));
		}
	}
	//imshow("minRGB",dc);
	//waitKey(1000);
	/****************************************/

	//choose radix
	/***************************************/
	int krnlsz;

	//if (w>h)
	//{
	//	krnlsz=(int)floor(maxnum(5,w*kenlRatio));
	//} 
	//else
	//{
	//	krnlsz=(int)floor(maxnum(5,h*kenlRatio));
	//}
	krnlsz = (int)floor(max(5.0, max(w*kenlRatio, h*kenlRatio)));
	/***************************************/

	/***************************************/

	double tmpmin;
	Mat tmp_img;
	Mat dc2;
	
	
	dc2.create(h, w, CV_8UC1);

	for (int i = 0; i<h; i++)
	{
		for (int j = 0; j<w; j++)
		{
			//getRectSubPix(dc_ex,Size(krnlsz,krnlsz),Point(i+krnlsz,j+krnlsz),tmp_img);
			getRectSubPix(dc, Size(krnlsz, krnlsz), Point(j, i), tmp_img);
			minMaxLoc(tmp_img, &tmpmin);
			dc2.at<uchar>(i, j) = (uchar)tmpmin;
			
		}
	}
	cuda::GpuMat x(dc2);

	//imshow("After filter",dc2);
	//waitKey(1000);
	/***************************************/
	Mat t;
	t.create(h, w, CV_8UC1);
	Mat M = Mat::ones(h, w, CV_8UC1);
	Mat M2 = M.mul(M, 255);
	cuda::GpuMat tg(t);
	cuda::GpuMat M3(M2);
	//cout<<M2<<endl;
	cuda::subtract(M3, x, tg);
//	subtract(M2, dc2, t);
	//imshow("t",t);
	//waitKey(1000);
	//cout<<(int)t.at<uchar>(0,0)<<endl;
	double maxval;
	//minMaxLoc(dc2, NULL, &maxval);
	cuda::minMaxLoc(x, NULL, &maxval, NULL, NULL);
	double A = min(minAtomsLight, (int)maxval);
	tg.download(t);
	t.convertTo(t, CV_64FC1, 1.f / 255);//convert t to[0,1]
	
	//cout<<t.at<double>(0,0)<<endl;
	Mat t_d = t.clone();//[0,1]
	cout << "The time: "<< t_d.at<double>(0, 0) << endl;	

	Mat J = Mat::zeros(h, w, CV_64FC3);

	img.convertTo(img, CV_64FC3);
	Mat img_d = img.clone();
	Mat tmp1;
	tmp1 = Mat::ones(h, w, CV_64FC1);
	cuda::GpuMat tmp1g(tmp1);
	cuda::GpuMat t_dg(t_d);
	cuda::subtract(tmp1g, t_dg, tmp1g);
	tmp1g.download(tmp1);
	//subtract(tmp1, t_d, tmp1);
	//cout<<tmp1.at<double>(0,0)<<endl;
	Mat tmp3 = tmp1.mul(tmp1, A);
	
	/*double tmpmin1;
	minMaxLoc(t_d,&tmpmin1);
	cout<<tmpmin1<<endl;*/
	for (int i = 0; i<h; i++)
	{
		for (int j = 0; j<w; j++)
		{
			if (t_d.at<double>(i, j)<0.1)
			{
				t_d.at<double>(i, j) = 0.1;
			}
			J.at<Vec3d>(i, j)[0] = (double)((double)img_d.at<Vec3d>(i, j)[0] - tmp3.at<double>(i, j)) / t_d.at<double>(i, j);
			J.at<Vec3d>(i, j)[1] = (double)((double)img_d.at<Vec3d>(i, j)[1] - tmp3.at<double>(i, j)) / t_d.at<double>(i, j);
			J.at<Vec3d>(i, j)[2] = (double)((double)img_d.at<Vec3d>(i, j)[2] - tmp3.at<double>(i, j)) / t_d.at<double>(i, j);
		}
	}
	//cout<<(double)img_d.at<Vec3d>(0,0)[0]<<endl;
	//cout<<tmp3.at<double>(0,0)<<endl;
	//cout<<t_d.at<double>(0,0)<<endl;
	//cout<<J.at<Vec3d>(0,0)[0]<<endl;
	//J.convertTo(J,CV_8UC3);
	//imshow("J",J);
	//waitKey(1000);
	int r = krnlsz * 4;//r is no less than  4x radix of minfilter
	double eps = 0.000001f;
	Mat gray;
	//imshow("1",imgcopy);
	//waitKey(1000);
	cvtColor(imgcopy, gray, CV_RGB2GRAY);
	gray.convertTo(gray, CV_64FC1, 1.f / 255);
	cuda::GpuMat filtered = guidedfilter(gray, t_d, r, eps);

	//t_d = filtered.clone();
	//cout << t_d.at<double>(0, 0) << endl;
	//t_d.convertTo(t_d,CV_8UC1,255);
	//imshow("t_d",t_d);
	//waitKey(1000);
	double minval1, maxval1;
	cuda::minMaxLoc(filtered, &minval1, &maxval1, NULL, NULL);
	cout << minval1 << endl;
	cout << maxval1 << endl;
	Mat tmp2 = Mat::ones(h, w, CV_64FC1);
	cuda::GpuMat tmp2g(tmp2);
	//t_dg.upload(filtered);
	cuda::subtract(tmp2g, filtered, tmp2g);
	tmp2g.download(tmp2);
	Mat tmp4 = tmp2.mul(tmp2, A);
	for (int i = 0; i<h; i++)
	{
		for (int j = 0; j<w; j++)
		{
			if (t_d.at<double>(i, j)<0.1)
			{
				t_d.at<double>(i, j) = 0.1;
			}
			J.at<Vec3d>(i, j)[0] = (double)((double)img_d.at<Vec3d>(i, j)[0] - tmp4.at<double>(i, j)) / t_d.at<double>(i, j);
			J.at<Vec3d>(i, j)[1] = (double)((double)img_d.at<Vec3d>(i, j)[1] - tmp4.at<double>(i, j)) / t_d.at<double>(i, j);
			J.at<Vec3d>(i, j)[2] = (double)((double)img_d.at<Vec3d>(i, j)[2] - tmp4.at<double>(i, j)) / t_d.at<double>(i, j);
		}
	}
	J.convertTo(J, CV_8UC3);
	//imshow("qqq",J);
	//waitKey(1000);

	return J;
}
int main(int argc, char * argv[])
{
	cout << "Number of cuda devices found: " << cuda::getCudaEnabledDeviceCount() << endl;
	cout << "Arg is " << argv[1] << endl;

	double kenlRatio = 0.01;
	int minAtomsLight = 220;//max
	////////////////////////////// Video capture
	//VideoCapture vid(0);


	////////////////////////
	/*Mat img;
	vid >> img;
	for (;;)
	{
	vid.read(img);
	}*/
	
	Mat img = imread(argv[1]);
	imshow("src", img);
	double t = (double)cvGetTickCount();
	Mat dst = dehaze_cplusplus(img, kenlRatio, minAtomsLight);
	t = (double)cvGetTickCount() - t;
	cout << "cost time: " << t / ((double)cvGetTickFrequency()*1000000.) << " s" << endl;
	cout << cvRound(((double)cvGetTickFrequency()*1000000.) / t) << "FPS" << endl;
	imwrite("out.jpg", dst);
	imshow("dehaze", dst);
	waitKey(0);

	cin.get();
	return 0;
}

