#include<iostream>
#include<vector>
#include<opencv2/core/core.hpp>
#include<opencv2/features2d/features2d.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/calib3d.hpp>
#include<opencv2/imgproc.hpp>
#include"laumemoryobject.h"


using namespace std;
using namespace cv;
using namespace libtiff;


int main()
{
	string filename = "ledIllumination.lau";
	LAUMemoryObject obj(filename);
	int frm1 = 1;
	/* we only want 2d img right now */
	int row = obj.height / 2;
	int slice = obj.frames;
	vector<Mat> spectral;
    Mat centerline(obj.height() / 2, obj.width(), CV_8UC1, (unsigned char *)obj.constScanLine(0, frm1), obj.step());
	/* still need to sum*/
	spectral.push_back(centerline);

	//first 2d image
	Mat img1(obj.height() / 2, obj.width(), CV_8UC1, (unsigned char *)obj.constScanLine(row, frm1), obj.step());
    //Mat img1 = imread(filename);
	int wid = img1.cols;
	int height = img1.rows;
	/* creat orb detect and initialize */
	Ptr<ORB> orb = ORB::create();
	vector<KeyPoint> points;
	Mat des;
	orb->detectAndCompute(img1, Mat(), points, des);
	
	/* initialize the homo matrix to store all homo*/
	vector<Mat> homo;

	for (int i = 2; i < slice; i++) {

		/* move the old image to the prev to ready for read new img */
		vector<KeyPoint> prevpoints;
		prevpoints = points;
		Mat prevdes;
		prevdes = des;
		int frm2 = i;
		/* read new img and detect */
		Mat centerline(obj.height() / 2, obj.width(), CV_8UC1, (unsigned char *)obj.constScanLine(0, frm2), obj.step());
		spectral.push_back(centerline);
		/* still need to sum */

		Mat img2(obj.height() / 2, obj.width(), CV_8UC1, (unsigned char *)obj.constScanLine(row, frm2), obj.step());
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
		homo[i] = findHomography(imgpoints2, imgpoints1, CV_RANSAC);

		homo[i] = homo[i] * homo[i - 1];
	}

	

	Mat panorama;
	int colnumber = img1.cols;
	/*
	for (int i = 0; i < slice; i++) {
		Mat a;
		Mat stitchedimg;
		reduce(spectral[i], a, 0, REDUCE_SUM);






	}
	*/
	
	for (int i = 0; i < slice; i++) {
		/* sum the hyperspectal to one row */
		Mat tmp;
		Mat stitchedimg;
		reduce(spectral[i], tmp, 0, REDUCE_SUM);

		uchar* data = tmp.ptr<uchar>(0);
		for (int j = 0; i < colnumber; i++) {
			Mat cor, rescor;
			cor.at<Vec2d>(0, 0) = j;
			cor.at<Vec2d>(1, 0) = height / 2 + 1;
			cor.at<Vec2d>(2, 0) = 1;

			/*
			vector<vector<int>> cor;
			cor[0].push_back(j);
			cor[1].push_back(height / 2 + 1);
			cor[2].push_back(1);
			vector<vector<int>> rescor;
			rescor[0][0] = rescor[0][0] / rescor[2][0];
			rescor[1][0] = rescor[1][0] / rescor[2][0];
			if (rescor[0][0] < 1 || rescor[1][0] < 1 || rescor[0][0] > 9999 || rescor[1][0] > 9999) {
				continue;
			}

	    	*/
			
			rescor = homo[i] * cor;

			//how to divide?


			
		


			panorama.at<Vec2d>(rescor[1][0], rescor[0][0]) = data[j];

		}
	}
	



	return 0;
}