#include<iostream>
#include<vector>
#include<opencv2/core/core.hpp>
#include<opencv2/features2d/features2d.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/calib3d.hpp>
#include<opencv2/imgproc.hpp>
//#include "tiffio.h"
//#include"laumemoryobject.h"


using namespace std;
using namespace cv;



int main()
{
	/*
	const char* filename = "ledIllumination.lau";
	TIFF * obj = TIFFOpen(filename, "r");
	int slice = TIFFNumberOfDirectories(obj);
	cout << slice << endl;
	char *dtitle;
	TIFFGetField(obj, TIFFTAG_PAGENAME, &dtitle);
	*/
	string filename = "res";
	string suf = ".jpg";
	int slice = 5;
	waitKey(0);
	int frm1 = 1;
	//TIFFGetField(obj, TIFFTAG_IMAGEWIDTH, &wid);
	//TIFFGetField(obj, TIFFTAG_IMAGELENGTH, &hei);
	Mat img = imread(filename + '1' + suf);


	int wid = img.cols;
	int hei = img.rows;
	Rect roit(0, 0, wid, hei / 2);
	Rect roib(0, hei / 2, wid, hei / 2);

	Mat centerline = img(roit);
	Mat img1 = img(roib);

	/* we only want 2d img right now */
	vector<Mat> spectral;
	//Mat centerline(obj.height() / 2, obj.width(), CV_8UC1, (unsigned char *)obj.constScanLine(0, frm1), obj.step());
	/* still need to sum*/
	spectral.push_back(centerline);

	//first 2d image
	//Mat img1(obj.height() / 2, obj.width(), CV_8UC1, (unsigned char *)obj.constScanLine(row, frm1), obj.step());
	//Mat img1 = imread(filename);

	/* creat orb detect and initialize */
	Ptr<ORB> orb = ORB::create();
	vector<KeyPoint> points;
	Mat des;
	orb->detectAndCompute(img1, Mat(), points, des);
	/* initialize the homo matrix to store all homo */
	vector<Mat> homo;
	Mat E = Mat::eye(3, 3, CV_64F);
	homo.push_back(E);
	for (int i = 2; i <= slice; i++) {
		/* move the old image to the prev to ready for read new img */
		vector<KeyPoint> prevpoints;
		prevpoints = points;
		Mat prevdes;
		prevdes = des;
		//int frm2 = i;
		/* read new img and detect */
		//Mat centerline(obj.height() / 2, obj.width(), CV_8UC1, (unsigned char *)obj.constScanLine(0, frm2), obj.step());

		Mat img = imread(filename + to_string(i) + suf);

		Mat img2 = img(roib);

		Mat centerline = img(roit);

		spectral.push_back(centerline);
		
		/* still need to sum */

		//Mat img2(obj.height() / 2, obj.width(), CV_8UC1, (unsigned char *)obj.constScanLine(row, frm2), obj.step());
		orb->detectAndCompute(img2, Mat(), points, des);

		/* match and pick good match*/
		vector<DMatch> match1to2, match2to1, goodmatch;
		Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce");
		matcher->match(prevdes, des, match1to2);
		matcher->match(des, prevdes, match2to1);

		int *flag = new int[des.rows];
		memset(flag, -1, sizeof(int) * des.rows);

		for (size_t i = 0; i < des.rows; i++) {

			flag[match2to1[i].queryIdx] = match2to1[i].trainIdx;

		}
		for (size_t i = 0; i < match1to2.size(); i++) {

			if (flag[match1to2[i].trainIdx] == match1to2[i].queryIdx) {
				goodmatch.push_back(match1to2[i]);

			}
		}

		/* 50 pair was enough, if its smaller, we still  accept it and store in res*/
		sort(goodmatch.begin(), goodmatch.end());
		size_t loop = goodmatch.size() * 0.15 > 50 ? 50 : goodmatch.size() * 0.15;
		vector<DMatch> res;
		for (int i = 0; i < loop; i++) {
			res.push_back(goodmatch[i]);
		}

		/* its check point */
		/*
		Mat ShowMatches;

		drawMatches(img1, prevpoints, img2, points, res, ShowMatches);

		imshow("matches", ShowMatches);
		*/

		vector<Point2f> imgpoints1, imgpoints2;
		for (int i = 0; i < res.size(); i++) {

			imgpoints1.push_back(prevpoints[res[i].queryIdx].pt);
			imgpoints2.push_back(points[res[i].trainIdx].pt);

		}
		homo.push_back(findHomography(imgpoints2, imgpoints1, CV_RANSAC));
		//cout << homo[i] << endl;
		//homo[i] = homo[i] * homo[i - 1];
		if (homo.size() > 2) {
			homo[i - 1] = homo[i - 1] * homo[i - 2];
		}
	}
	Mat panorama(hei, wid, CV_32F);
	
	// all clear
	for (int i = 0; i < slice; i++) {
		/* sum the hyperspectal to one row */
		Mat tmp;
		Mat stitchedimg;
		reduce(spectral[i], tmp, 0, CV_REDUCE_SUM, CV_32F); //
		float* data = tmp.ptr<float>(0);
		for (int j = 0; j < wid; j++) {
			Mat cor, rescor;
			cor = (Mat_<double>(3, 1) << j, hei / 4 + 1, 1);
			rescor = homo[i] * cor; 	
			double c1 = rescor.at<double>(0, 0);
			double r1 = rescor.at<double>(1, 0);
			double divi = rescor.at <double>(2, 0);
			c1 = floor(c1 / divi);
			r1 = floor(r1 / divi);
			
			
			
			if (c1 < 1 || r1 < 1 || c1 > wid || r1 > hei) {
				continue;
			}
			panorama.at<float>(r1, c1) = data[j];
			//cout << panorama.at<float>(r1, c1) << endl;
			//cin.get();
		}
		//cin.get();
		
	}
	
	return 0;
}