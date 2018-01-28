#include <atomic>
#include <iostream>
#include "helper.h"
#include <thread>
// GLOBAL VARS
VerySimpleThreadSafeFIFOBuffer<MyData> theBuffer(200);
atomic<bool> grabbing, processing;
void GrabberThread_Test();
void ProcessorThread_Test();
void ProcessDataInThread_Test(MyData &data);
void do_work(MyData &data);

// THE GRABBER THREAD
void GrabberThread_Test()
{
	processing.store(true);    //ensure to start the processing

	VideoCapture capIr, capDepth;
	MyData grabData;
	const char* path = "/home/kris/auto-drive/short.mp4";
	capIr.open(path);
	//capDepth.open(1);
	//cap...
	int frameCount = 0;
	int retry = 50;
	int interval = 100;
	while (grabbing.load() == true) //this is lock free
	{
		//WORK WITH grabData OBJECT
		capIr >> grabData.ir;
		//capDepth >> grabData.depth
		//...

		if (grabData.ir.empty()) continue;
		frameCount++;
		grabData.vars.timestamp = getTickCount();
		grabData.vars.frameNum = frameCount;
		
		//if (!theBuffer.Push(grabData))
		//{
		//	cout << "GrabberThread_Test: " << endl
		//		<< "The queue is full! Grabber is going to sleep a bit"<<endl
		//		<< "HINT: increase queue size or improve processor performance or reduce FPS" 
		//		<< endl;
		//	this_thread::sleep_for(chrono::milliseconds(delay));
		//	delay += 80;
		//}
		//else
		//{
		//	if (delay>defaultDelay)
		//		cout << "GrabberThread_Test: " << "Wake up !" << endl;
		//	delay = defaultDelay;
		//}
		
		if (theBuffer.Push(grabData))
		{
			// Every interval milliseconds push a frame into the buffer
			this_thread::sleep_for(chrono::milliseconds(interval));
		}
		else
		{
			cout << "Buffer is full" << endl;
			while(!theBuffer.Push(grabData)){
				this_thread::sleep_for(chrono::milliseconds(retry));
			}
		}

	}
	processing.store(false);    //!!!!!!stop processing here
}

//THE PROCESSOR THREAD
//void ProcessorThread_Test()
//{
//	MyData procData;
//	Mat ir, depth;
//	while (processing.load() == true) //this is lock free
//	{
//		//this is same of procData="front of the queue" and "remove front"
//		//cv::Mats in procData are still valid because of cv::Mat internal counter
//		if (theBuffer.Pop(procData) == false) continue;
//		ProcessDataInThread_Test(procData);
//	}
//	size_t todo = theBuffer.Size();
//	cout << endl << "ProcessorThread: Flushing buffer: "
//		<< todo << " frames..." << endl;
//	while (theBuffer.Pop(procData))
//	{
//		ProcessDataInThread_Test(procData);
//		cout << "todo: " << todo-- << endl;
//	}
//	cout << "ProcessorThread: done!" << endl;
//}

void ProcessorThread_Test()
{
	MyData procData;
	Mat ir, depth;
	while (processing.load() == true) //this is lock free
	{
		//this is same of procData="front of the queue" and "remove front"
		//cv::Mats in procData are still valid because of cv::Mat internal counter
		if (theBuffer.Pop(procData) == false) continue;

		int len = theBuffer.PopAndCount(procData);
		if(len ==-1){ // if empty buffer
			continue;
		}else{
			if(len == 0){
				cout << "Meet real-time"  << endl;
			}else{
				cout << "Miss deadline" << endl;
			}
			do_work(procData);
		}

	}
	size_t todo = theBuffer.Size();
	cout << endl << "ProcessorThread: Flushing buffer: "
		<< todo << " frames..." << endl;
	while (theBuffer.Pop(procData))
	{
		ProcessDataInThread_Test(procData);
		cout << "todo: " << todo-- << endl;
	}
	cout << "ProcessorThread: done!" << endl;
}

void do_work(MyData &data){
	int sleep = 200;
	this_thread::sleep_for(chrono::milliseconds(sleep));
}

//THE PROCESSING FUNCTION CALLED BY PROCESSING THREAD
void ProcessDataInThread_Test(MyData &data)
{
	//int size = 1;
	//Point anchor(size, size);
	//Size sz(2 * size + 1, 2 * size + 1);
	//Mat element = getStructuringElement(MORPH_ELLIPSE, sz, anchor);
	//morphologyEx(data.ir, data.ir, MORPH_GRADIENT, element, anchor);
	//putText(data.ir, to_string(data.vars.frameNum),
	//		Point(10, 10), 1, 1, Scalar(0, 255, 0));
	imshow("IR", data.ir);
	//imshow("DEPTH", data.depth);
	//..
	waitKey(1);
}

//MAIN THREAD
//int MainQueueGrabbingThread() {
int main() {

	grabbing.store(true);               // set the grabbing control variable
	processing.store(true);             // ensure the processing will start
	thread grab(GrabberThread_Test);    // start the grabbing task
	thread proc(ProcessorThread_Test);  // start the the processing task

	//your own GUI
	cout << endl << "Press Enter to stop grabbing...";
	cin.get();

	// terminate all
	grabbing.store(false);    // stop the grab loop 
	processing.store(false);  // ensure to stop the processing
	grab.join();              // wait for the grab thread
	proc.join();              // wait for the process thread

#ifdef _DEBUG
	//get some stats on the buffer
	cout << "Number Queued items: " << theBuffer.GetItemCount() << endl;
	cout << "Queue Max Size: " << theBuffer.GetBufSizeMax() << endl;
	cout << "Number of Data Object in memory: " << theBuffer.GetDataMemoryCount() << endl;
	cout << "Number of Mat Object in memory: " << theBuffer.GetMatMemoryCount() << endl;
	cout << endl << "Press Enter to close !";
#endif
	cin.get();
	return 0;
}
