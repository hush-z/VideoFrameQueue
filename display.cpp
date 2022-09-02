#include "opencv2/opencv.hpp"
#include <iostream>
#include <string>
using namespace cv;
using namespace std;
int main(int argc, char** argv)
{
	const char* path = "/home/kris/auto-drive/short.mp4";
	VideoCapture cap(path); // open the video file
	if(!cap.isOpened()){
		cout << "Open fail" << endl;
		return -1;
	}

	double fps = cap.get(CV_CAP_PROP_FPS);
	cout << "Frame per seconds : " << fps << endl;
	
	namedWindow("output");
	Mat frame;
	int numFrames=0;
   	while(cap.read(frame)){
		imshow("output", frame);
		waitKey(1);
		numFrames++;	
	}
	cout << "numFrames: " << numFrames << endl;
	for(;;)
	{
		Mat frame;
		cap >> frame; // get a new frame from camera        
		imshow("output", frame);
		if(waitKey(100) >= 0) break;
	}
	return 0;
}
